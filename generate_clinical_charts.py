import os
import sys
import torch
import torch.nn as nn
import numpy as np
import json
import time
import math
from datetime import datetime

# --- กำหนดค่า Device สำหรับประมวลผล ---
device = torch.device("cuda" if torch.cuda.is_available() else "cpu")

class PMaxProductionCore(nn.Module):
    """
    P'MAX Production Core Engine (Cardiac_Backend Version)
    จัดการระบบคำนวณทางสรีรวิทยา, โมเดล PyTorch ภายในโฟลเดอร์ Cardiac_Backend และ Ledger
    """
    def __init__(self, sample_rate=2000, backend_dir="Cardiac_Backend", calibration_duration_sec=30.0):
        super().__init__()
        self.sr = sample_rate
        self.backend_dir = backend_dir
        
        # กำหนดพาธภายใน Cardiac_Backend อย่างแม่นยำ
        self.models_dir = os.path.join(self.backend_dir, "models")
        self.ledger_file = os.path.join(self.backend_dir, "medical_blockchain_ledger.json")
        self.state_file = os.path.join(self.backend_dir, "pmax_calibration_state.json")
        
        # สร้างโฟลเดอร์ models หากยังไม่มี
        os.makedirs(self.models_dir, exist_ok=True)
        
        self.history_window = []
        self.window_limit_seconds = 300  # หน้าต่างประวัติย้อนหลัง 5 นาที (300 วินาที)
        
        # --- ระบบ Patient-Specific Baseline & Delta Calibration ---
        self.calibration_duration_sec = calibration_duration_sec
        self.is_calibrated = False
        
        self.calib_sys_buffer = []
        self.calib_dia_buffer = []
        self.calib_vortex_buffer = []
        
        self.sys_baseline = 0.0
        self.dia_baseline = 0.0
        self.vortex_baseline = 0.0
        
        # โหลดสถานะเดิม (ถ้ามี) และโมเดล PyTorch
        self._load_persistent_state()
        self.model = self.load_production_weights()

    def load_production_weights(self):
        model_path = os.path.join(self.models_dir, "model_weights_fold1.pth")
        if os.path.exists(model_path):
            try:
                loaded_obj = torch.load(model_path, map_location=device)
                if isinstance(loaded_obj, dict):
                    return None
                
                if hasattr(loaded_obj, 'eval'):
                    loaded_obj.eval()
                return loaded_obj
            except Exception:
                return None
        else:
            return None

    def _load_persistent_state(self):
        """กู้คืนค่า Calibration จาก Cardiac_Backend หากเคยบันทึกไว้"""
        if os.path.exists(self.state_file):
            try:
                with open(self.state_file, "r", encoding="utf-8") as f:
                    state = json.load(f)
                    self.sys_baseline = state.get("sys_baseline", 0.0)
                    self.dia_baseline = state.get("dia_baseline", 0.0)
                    self.vortex_baseline = state.get("vortex_baseline", 0.0)
                    self.is_calibrated = state.get("is_calibrated", False)
            except Exception:
                pass

    def _save_persistent_state(self):
        """บันทึกสถานะ Calibration ลง Cardiac_Backend"""
        try:
            state = {
                "sys_baseline": self.sys_baseline,
                "dia_baseline": self.dia_baseline,
                "vortex_baseline": self.vortex_baseline,
                "is_calibrated": self.is_calibrated,
                "updated_at": datetime.now().isoformat()
            }
            with open(self.state_file, "w", encoding="utf-8") as f:
                json.dump(state, f, indent=4)
        except Exception:
            pass

    def gammatone_filterbank_envelope(self, audio_chunk):
        if len(audio_chunk) == 0:
            return np.zeros(1)
        
        channels = 64
        t = np.arange(len(audio_chunk)) / self.sr
        envelope_channels = np.zeros((channels, len(audio_chunk)))
        
        for k in range(channels):
            center_freq = 50.0 + (k * 30.0)
            carrier = np.cos(2 * np.pi * center_freq * t)
            envelope_channels[k, :] = np.abs(audio_chunk * carrier)
            
        e_t = np.sqrt(np.maximum(np.sum(envelope_channels**2, axis=0), 0.0))
        return e_t

    def extract_exact_features(self, audio_chunk, patient_profile):
        if len(audio_chunk) == 0:
            return 0.0, 0.0, 0.0, 0.0, 1.0

        dt = 1.0 / self.sr
        e_t = self.gammatone_filterbank_envelope(audio_chunk)
        e_diff = np.abs(np.diff(e_t, prepend=e_t[0]))

        age = patient_profile.get('age', 40)
        age_ref = patient_profile.get('age_ref', 50.0) 
        bsa = patient_profile.get('bsa', 1.73)
        bsa_ref = 1.73 
        
        t_param = patient_profile.get('t_param', 0.1)
        c_dm = patient_profile.get('c_dm', 0.1)
        
        v_int_z = patient_profile.get('v_int_z', 10.0)
        v_wh = patient_profile.get('v_wh', 4.0)

        blood_viscosity = patient_profile.get('blood_viscosity', 3.5)
        vessel_length = patient_profile.get('vessel_length', 10.0)
        vessel_radius = patient_profile.get('vessel_radius', 1.0)
        pressure_gradient = patient_profile.get('pressure_gradient', 80.0)

        safe_radius = max(0.1, vessel_radius)
        vascular_resistance = (blood_viscosity * vessel_length) / (safe_radius ** 4)
        hemodynamic_flow_q = pressure_gradient / (vascular_resistance + 1e-6)
        flow_correction_factor = np.clip(hemodynamic_flow_q / 50.0, 0.5, 2.0)

        peak_filling = np.max(e_t) if len(e_t) > 0 else 1.0
        turbulent_variance = np.var(e_diff) if len(e_diff) > 0 else 0.1
        vortex_efficiency = peak_filling / (1.0 + turbulent_variance)
        vortex_efficiency = float(np.clip(vortex_efficiency, 0.1, 2.0))

        bsa_factor = bsa / bsa_ref
        physiological_factor = (1.0 + t_param) / (1.0 + c_dm)
        sys_riemann = np.sum(e_t * dt)
        system_gain_sys = 50.0
        
        i_sys = bsa_factor * physiological_factor * sys_riemann * system_gain_sys * flow_correction_factor

        base_diastolic_term = v_int_z + (np.sqrt(v_wh) * 166.67)
        exponential_age_decay = np.exp(-(age / age_ref))
        dia_riemann = np.sum(e_diff)
        
        i_dia = base_diastolic_term * exponential_age_decay * dia_riemann * (1.0 / vortex_efficiency)

        f_sys = np.clip(i_sys, 0.0, 100.0)
        f_dia = np.clip(i_dia, 0.0, 100.0)

        f_prime_sys = np.mean(e_diff) / dt
        t_array = np.arange(len(e_t)) * dt
        lam = 0.05 * vortex_efficiency
        integrand = np.exp(-lam * t_array) * e_diff
        
        if hasattr(np, 'trapezoid'):
            frank_starling_integral = np.trapezoid(integrand, t_array)
        elif hasattr(np, 'trapz'):
            frank_starling_integral = np.trapz(integrand, t_array)
        else:
            frank_starling_integral = float(np.sum((integrand[:-1] + integrand[1:]) * np.diff(t_array) / 2.0))

        return float(f_sys), float(f_dia), float(f_prime_sys), float(frank_starling_integral), float(vortex_efficiency)

    def evaluate_frank_starling_system(self, f_sys, f_dia, fs_integral, vortex_efficiency, patient_profile, recorded_time: str, location: str):
        current_time = time.time()
        ht = patient_profile.get('hypertension', False)
        dm = patient_profile.get('diabetes', False)
        
        risk_score = (f_sys * (1.5 if ht else 1.0) * (1.5 if dm else 1.0)) + f_dia
        
        y_max = 100.0
        lam = 0.05 * vortex_efficiency
        fs_slope = lam * max(0.0, y_max - f_sys) * f_dia * (1.0 + abs(fs_integral))
        
        # --- Phase 1: Onboarding Baseline Calibration ---
        if not self.is_calibrated:
            self.calib_sys_buffer.append(f_sys)
            self.calib_dia_buffer.append(f_dia)
            self.calib_vortex_buffer.append(vortex_efficiency)
            
            if len(self.calib_sys_buffer) >= max(10, int(self.calibration_duration_sec)):
                self.sys_baseline = float(np.mean(self.calib_sys_buffer))
                self.dia_baseline = float(np.mean(self.calib_dia_buffer))
                self.vortex_baseline = float(np.mean(self.calib_vortex_buffer))
                self.is_calibrated = True
                self._save_persistent_state()
            
            return {
                "Status": "CALIBRATING",
                "Progress": len(self.calib_sys_buffer) / int(self.calibration_duration_sec),
                "F_sys": float(f_sys),
                "F_dia": float(f_dia),
                "Vortex_Efficiency": float(vortex_efficiency),
                "Risk_Tier": "Calibrating",
                "Prediction": "System is learning patient's intrinsic baseline...",
                "SOS_Trigger": False,
                "Recorded_Time": recorded_time,
                "Location": location
            }

        # --- Phase 2: Acute Delta Tracking (35% Threshold) ---
        delta_sys_pct = (f_sys - self.sys_baseline) / (self.sys_baseline + 1e-6)
        delta_dia_pct = (f_dia - self.dia_baseline) / (self.dia_baseline + 1e-6)
        
        acute_threshold = 0.35
        is_acute_delta_exceeded = (abs(delta_sys_pct) > acute_threshold) or (abs(delta_dia_pct) > acute_threshold)

        is_abnormal_base = (f_sys > 80.0) or (fs_slope < 1.0) or (risk_score > 150.0) or (vortex_efficiency < 0.4)
        is_abnormal = is_abnormal_base and is_acute_delta_exceeded
        
        self.history_window.append({
            "timestamp": current_time,
            "recorded_time": recorded_time,
            "location": location,
            "is_abnormal": is_abnormal,
            "risk_score": risk_score,
            "fs_slope": fs_slope,
            "vortex_efficiency": vortex_efficiency
        })
        
        self.history_window = [
            item for item in self.history_window 
            if current_time - item["timestamp"] <= self.window_limit_seconds
        ]
        
        total = len(self.history_window)
        abnormal_count = sum(1 for item in self.history_window if item["is_abnormal"])
        freq_rate = (abnormal_count / total) if total > 0 else 0.0
        avg_risk = np.mean([item["risk_score"] for item in self.history_window]) if total > 0 else risk_score
        danger_index = avg_risk * (1.0 + freq_rate * 2.0)
        
        tier, prediction = "Low", "Stable"
        sos_trigger = False
        
        if danger_index > 200.0 and freq_rate > 0.6:
            tier, prediction = "High Critical", "Warning: Acute hemodynamic crisis / Vortex collapse detected!"
            sos_trigger = True
        elif danger_index > 120.0 and freq_rate > 0.3:
            tier, prediction = "Moderate Risk", "Caution: Elevated fatigue / LA dysfunction trend."
            
        return {
            "Status": "MONITORING_ACTIVE",
            "Recorded_Time": recorded_time,
            "Location": location,
            "F_sys": float(f_sys),
            "F_dia": float(f_dia),
            "Vortex_Efficiency": float(vortex_efficiency),
            "Delta_Sys_Pct": float(delta_sys_pct),
            "Delta_Dia_Pct": float(delta_dia_pct),
            "Frank_Starling_Slope": float(fs_slope),
            "FS_Integral": float(fs_integral),
            "Risk_Score": float(risk_score),
            "Danger_Index": float(danger_index),
            "Freq_Rate": float(freq_rate),
            "Risk_Tier": tier,
            "Prediction": prediction,
            "SOS_Trigger": bool(sos_trigger)
        }

    def log_to_blockchain_ledger(self, evaluation_result):
        ledger_entry = {
            "timestamp": datetime.now().isoformat(),
            "status": "VERIFIED_PRODUCTION",
            "result": evaluation_result,
            "node_id": "PMAX-BACKEND-NODE"
        }
        
        ledger_data = []
        if os.path.exists(self.ledger_file):
            with open(self.ledger_file, "r", encoding="utf-8") as f:
                try:
                    ledger_data = json.load(f)
                except json.JSONDecodeError:
                    ledger_data = []
                    
        ledger_data.append(ledger_entry)
        with open(self.ledger_file, "w", encoding="utf-8") as f:
            json.dump(ledger_data, f, indent=4)