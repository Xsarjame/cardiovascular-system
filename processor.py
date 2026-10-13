import os
import glob
import numpy as np
import torch
import torch.nn as nn
import time
import math

class ExactMathCardiacPipeline(nn.Module):
    def __init__(self, sample_rate=2000, base_dir="./models", calibration_duration_sec=30.0):
        super().__init__()
        self.sr = sample_rate
        self.base_dir = base_dir
        self.history_window = []
        self.window_limit_seconds = 300  # เก็บประวัติย้อนหลัง 5 นาที
        
        # --- ตัวแปรสำหรับระบบ Patient-Specific Baseline & Delta Calibration ---
        self.calibration_duration_sec = calibration_duration_sec
        self.calib_samples = int(calibration_duration_sec * sample_rate)
        self.is_calibrated = False
        
        # Buffers สำหรับเก็บค่าช่วง Onboarding Calibration รายบุคคล
        self.calib_sys_buffer = []
        self.calib_dia_buffer = []
        self.calib_vortex_buffer = []
        
        # ค่า Baseline ส่วนตัวที่จะถูกบันทึกหลังผ่านช่วงคาลิเบรต
        self.sys_baseline = 0.0
        self.dia_baseline = 0.0
        self.vortex_baseline = 0.0
        
        # โหลดโมเดล PyTorch Folds 1 ถึง 5 ครบถ้วน (Ensemble)
        self.models = []
        for i in range(1, 6):
            model_path = os.path.join(self.base_dir, f"best_gammatone_model_fold_{i}.pth")
            if os.path.exists(model_path):
                try:
                    m = torch.load(model_path, map_location=torch.device("cpu"))
                    if hasattr(m, 'eval'):
                        m.eval()
                    self.models.append(m)
                except Exception:
                    pass
        print(f"🚀 [Processor] โหลด PyTorch Ensembles สำเร็จ {len(self.models)} โฟลด์ (Fold 1 - 5)")

    def gammatone_filterbank_envelope(self, audio_chunk):
        """
        ถอดสมการ: E(t) = sqrt(sum_{k=1}^{64} [X_k(t)]^2)
        """
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
        """
        คำนวณดัชนีทางสรีรวิทยาตามสมการฉบับแก้ไข (Modified Systolic & Diastolic Indices)
        พร้อมผสาน Hemodynamic Fluid Dynamics (Q = Delta P / R, R proportional to eta * L / r^4)
        และ Ventricular Filling Vortex Dynamics (Vortex Efficiency Index Omega)
        """
        if len(audio_chunk) == 0:
            return 0.0, 0.0, 0.0, 0.0, 1.0

        dt = 1.0 / self.sr
        e_t = self.gammatone_filterbank_envelope(audio_chunk)
        
        # Differential Energy สำหรับช่วง Diastolic Decay และการวิเคราะห์ Vortex
        e_diff = np.abs(np.diff(e_t, prepend=e_t[0]))

        # ดึงพารามิเตอร์ผู้ป่วยและกำหนดค่ามาตรฐาน (Dimensionless & Normalized)
        age = patient_profile.get('age', 40)
        age_ref = patient_profile.get('age_ref', 50.0) 
        bsa = patient_profile.get('bsa', 1.73)
        bsa_ref = 1.73  # m^2 ค่ามาตรฐาน
        
        t_param = patient_profile.get('t_param', 0.1)    # T_hat (Dimensionless [0, 1])
        c_dm = patient_profile.get('c_dm', 0.1)          # C_DM (Cardiac Dysfunction Marker [0, 1])
        
        v_int_z = patient_profile.get('v_int_z', 10.0)
        v_wh = patient_profile.get('v_wh', 4.0)

        # --- EXTENSION: Hemodynamic Fluid Dynamics & Vortex Parameters ---
        blood_viscosity = patient_profile.get('blood_viscosity', 3.5)   # cP (ความหนืดของเลือด eta)
        vessel_length = patient_profile.get('vessel_length', 10.0)      # cm (ความยาว L)
        vessel_radius = patient_profile.get('vessel_radius', 1.0)       # cm (รัศมีหลอดเลือด r)
        pressure_gradient = patient_profile.get('pressure_gradient', 80.0) # mmHg (Delta P)

        # คำนวณความต้านทานหลอดเลือด (Vascular Resistance R proportional to eta * L / r^4)
        # ตามกฎ Poiseuille: R = (8 * eta * L) / (pi * r^4) -> ใช้สัดส่วนเชิงฟิสิกส์
        safe_radius = max(0.1, vessel_radius)
        vascular_resistance = (blood_viscosity * vessel_length) / (safe_radius ** 4)
        
        # อัตราการไหลของเลือด (Blood Flow Q = Delta P / R)
        hemodynamic_flow_q = pressure_gradient / (vascular_resistance + 1e-6)
        flow_correction_factor = np.clip(hemodynamic_flow_q / 50.0, 0.5, 2.0)

        # คำนวณ Vortex Efficiency Index (Omega) จากความสมบูรณ์ของพลังงานช่วง Diastolic Filling
        peak_filling = np.max(e_t) if len(e_t) > 0 else 1.0
        turbulent_variance = np.var(e_diff) if len(e_diff) > 0 else 0.1
        vortex_efficiency = peak_filling / (1.0 + turbulent_variance)
        vortex_efficiency = float(np.clip(vortex_efficiency, 0.1, 2.0))

        # 1. คำนวณ Modified Systolic Work Index (I_sys) ผสาน Hemodynamic Flow Q
        bsa_factor = bsa / bsa_ref
        physiological_factor = (1.0 + t_param) / (1.0 + c_dm)
        sys_riemann = np.sum(e_t * dt)
        
        system_gain_sys = 50.0  # System Gain สำหรับแปลงหน่วยพลังงานทางกายภาพ
        i_sys = bsa_factor * physiological_factor * sys_riemann * system_gain_sys * flow_correction_factor

        # 2. คำนวณ Modified Diastolic Decay Index (I_dia) ด้วย Exponential Decay และ Vortex Efficiency
        base_diastolic_term = v_int_z + (np.sqrt(v_wh) * 166.67)
        exponential_age_decay = np.exp(-(age / age_ref))
        dia_riemann = np.sum(e_diff)
        
        i_dia = base_diastolic_term * exponential_age_decay * dia_riemann * (1.0 / vortex_efficiency)

        # จำกัดขอบเขตให้อยู่ในช่วง 0.0 - 100.0
        f_sys = np.clip(i_sys, 0.0, 100.0)
        f_dia = np.clip(i_dia, 0.0, 100.0)

        # คำนวณเทอมเสริมสำหรับ Frank-Starling Integration
        f_prime_sys = np.mean(e_diff) / dt
        t_array = np.arange(len(e_t)) * dt
        lam = 0.05 * vortex_efficiency  # ปรับอัตราการตอบสนองตามประสิทธิภาพกระแสไหลวน
        integrand = np.exp(-lam * t_array) * e_diff
        frank_starling_integral = np.trapz(integrand, t_array)

        return float(f_sys), float(f_dia), float(f_prime_sys), float(frank_starling_integral), float(vortex_efficiency)

    def evaluate_frank_starling_system(self, f_sys, f_dia, fs_integral, vortex_efficiency, patient_profile, recorded_time: str = "--", location: str = "--"):
        """
        ระบบประเมินผลเชิงคลินิก ผสาน Patient-Specific Baseline Calibration & Rate of Change
        ร่วมกับผลลัพธ์จาก Hemodynamic และ Vortex Dynamics
        """
        current_time = time.time()
        ht = patient_profile.get('hypertension', False)
        dm = patient_profile.get('diabetes', False)
        
        risk_score = (f_sys * (1.5 if ht else 1.0) * (1.5 if dm else 1.0)) + f_dia
        
        y_max = 100.0
        lam = 0.05 * vortex_efficiency
        fs_slope = lam * max(0.0, y_max - f_sys) * f_dia * (1.0 + abs(fs_integral))
        
        # --- PHASE 1: Onboarding Baseline Calibration (ป้องกัน False Positive สำหรับคนลิ้นหัวใจรั่ว) ---
        if not self.is_calibrated:
            self.calib_sys_buffer.append(f_sys)
            self.calib_dia_buffer.append(f_dia)
            self.calib_vortex_buffer.append(vortex_efficiency)
            
            # ตรวจสอบว่าเก็บข้อมูลครบตามเวลาที่กำหนดหรือยัง (เช่น 30 วินาทีแรก)
            if len(self.calib_sys_buffer) >= max(10, int(self.calibration_duration_sec)):
                self.sys_baseline = float(np.mean(self.calib_sys_buffer))
                self.dia_baseline = float(np.mean(self.calib_dia_buffer))
                self.vortex_baseline = float(np.mean(self.calib_vortex_buffer))
                self.is_calibrated = True
                print(f"✅ [Calibration Success] จดจำค่าพื้นฐานส่วนตัวสำเร็จ -> Baseline Sys: {self.sys_baseline:.2f}, Dia: {self.dia_baseline:.2f}, Vortex: {self.vortex_baseline:.2f}")
            
            return {
                "Status": "CALIBRATING",
                "Progress": len(self.calib_sys_buffer) / int(self.calibration_duration_sec),
                "F_sys": float(f_sys),
                "F_dia": float(f_dia),
                "Vortex_Efficiency": float(vortex_efficiency),
                "Risk_Tier": "Calibrating",
                "Prediction": "System is learning patient's intrinsic baseline (Murmur/Valve profile)...",
                "SOS_Trigger": False,
                "Recorded_Time": recorded_time,
                "Location": location
            }

        # --- PHASE 2: Acute Rate of Change (Delta Tracking) เทียบกับ Baseline ส่วนตัว ---
        delta_sys_pct = (f_sys - self.sys_baseline) / (self.sys_baseline + 1e-6)
        delta_dia_pct = (f_dia - self.dia_baseline) / (self.dia_baseline + 1e-6)
        
        # กำหนด Threshold ความเปลี่ยนแปลงฉับพลัน (เช่น พุ่งกระโดดเกิน 35% จากค่าปกติส่วนตัว)
        acute_threshold = 0.35
        is_acute_delta_exceeded = (abs(delta_sys_pct) > acute_threshold) or (abs(delta_dia_pct) > acute_threshold)

        # เงื่อนไขความผิดปกติแบบดั้งเดิมร่วมกับ Delta Check
        is_abnormal_base = (f_sys > 80.0) or (fs_slope < 1.0) or (risk_score > 150.0) or (vortex_efficiency < 0.4)
        
        # บังคับว่าต้องเป็นการเปลี่ยนแปลงฉับพลัน (Acute Change) หรือวิกฤตจริงเท่านั้นถึงจะนับว่าผิดปกติ
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
            tier, prediction = "High Critical", "Warning: Acute hemodynamic crisis / Vortex collapse / Heart Failure trend detected!"
            sos_trigger = True  # สั่งแอปฯ โทรออก 1 ครั้ง + ส่งพิกัด GPS ทันที
        elif danger_index > 120.0 and freq_rate > 0.3:
            tier, prediction = "Moderate Risk", "Caution: Elevated fatigue / LA dysfunction trend within 1 - 2 hours."
            
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
            "Risk_Tier": tier,
            "Prediction": prediction,
            "SOS_Trigger": bool(sos_trigger)
        }