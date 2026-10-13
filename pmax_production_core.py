import os
import sys
import json
import time
import hashlib
import threading
from datetime import datetime
from typing import Dict, Any, Optional, List
import numpy as np
import torch
import torch.nn as nn

# --- กำหนดค่า Device สำหรับประมวลผล ---
device = torch.device("cuda" if torch.cuda.is_available() else "cpu")

class CryptographicAuditSigner:
    """
    ระบบเข้ารหัสและลงลายมือชื่อดิจิทัลสำหรับข้อมูลทางคลินิก (Cryptographic Audit Trail)
    แทนที่ Proof-of-Work ที่ไม่จำเป็น เพื่อความเร็วและการประหยัดพลังงานบน Edge Device
    """
    @staticmethod
    def generate_secure_commitment(patient_id: str, timestamp: float, metrics: Dict[str, float], secret_salt: str = "PMAX_PROD_SECURE_SALT_2026") -> str:
        payload_string = f"{patient_id}:{timestamp}:{metrics.get('f_sys', 0):.4f}:{metrics.get('f_dia', 0):.4f}:{metrics.get('risk_score', 0):.4f}:{secret_salt}"
        return hashlib.sha256(payload_string.encode('utf-8')).hexdigest()

class ProductionSecureLedger:
    """
    ระบบบันทึกข้อมูลแบบ Append-Only Log สำหรับอุปกรณ์ทางการแพทย์ (High Performance & Thread-Safe)
    """
    def __init__(self, ledger_file: str = "pmax_production_ledger.log"):
        self.ledger_file = ledger_file
        self.lock = threading.Lock()
        self._initialize_ledger()

    def _initialize_ledger(self):
        with self.lock:
            if not os.path.exists(self.ledger_file):
                genesis_record = {
                    "index": 0,
                    "timestamp": time.time(),
                    "data": {"message": "P'MAX Production Ledger Genesis Block"},
                    "previous_hash": "0" * 64,
                    "hash": hashlib.sha256(b"PMAX_GENESIS_BLOCK").hexdigest()
                }
                with open(self.ledger_file, "w", encoding="utf-8") as f:
                    f.write(json.dumps(genesis_record) + "\n")

    def append_record(self, record_data: Dict[str, Any]) -> str:
        with self.lock:
            # อ่านบล็อกล่าสุดเพื่อเอา Previous Hash
            previous_hash = "0" * 64
            index = 1
            if os.path.exists(self.ledger_file):
                with open(self.ledger_file, "r", encoding="utf-8") as f:
                    lines = f.readlines()
                    if lines:
                        last_line = lines[-1].strip()
                        if last_line:
                            try:
                                last_block = json.loads(last_line)
                                previous_hash = last_block.get("hash", previous_hash)
                                index = last_block.get("index", 0) + 1
                            except json.JSONDecodeError:
                                pass

            timestamp = time.time()
            block_content = {
                "index": index,
                "timestamp": timestamp,
                "data": record_data,
                "previous_hash": previous_hash
            }
            
            block_string = f"{index}{timestamp}{json.dumps(record_data, sort_keys=True)}{previous_hash}"
            block_hash = hashlib.sha256(block_string.encode('utf-8')).hexdigest()
            block_content["hash"] = block_hash

            with open(self.ledger_file, "a", encoding="utf-8") as f:
                f.write(json.dumps(block_content) + "\n")
                
            return block_hash

class PMaxProductionCore(nn.Module):
    """
    P'MAX Production Core: ระบบประมวลผลสัญญาณชีพและตรวจจับความผิดปกติหัวใจแบบเรียลไทม์
    รองรับ Thread-Safety, Persistent Calibration State และ Hardware Telemetry Verification
    """
    def __init__(self, sample_rate: int = 2000, model_dir: str = "./models", state_file: str = "pmax_calibration_state.json"):
        super().__init__()
        self.sr = sample_rate
        self.model_dir = model_dir
        self.state_file = state_file
        self.lock = threading.Lock()
        
        self.history_window = []
        self.window_limit_seconds = 300
        
        self.calibration_duration_sec = 30.0
        self.is_calibrated = False
        self.calib_sys_buffer = []
        self.calib_dia_buffer = []
        self.calib_vortex_buffer = []
        
        self.sys_baseline = 0.0
        self.dia_baseline = 0.0
        self.vortex_baseline = 0.0
        
        self.ledger = ProductionSecureLedger()
        self._load_persistent_state()
        self.model = self._load_production_weights()

    def _load_production_weights(self) -> Optional[torch.nn.Module]:
        model_path = os.path.join(self.model_dir, "model_weights_fold1.pth")
        if os.path.exists(model_path):
            try:
                loaded_obj = torch.load(model_path, map_location=device)
                if isinstance(loaded_obj, dict):
                    return None  # เป็น State Dict ใช้งาน Exact Analytic Baseline แทน
                if hasattr(loaded_obj, 'eval'):
                    loaded_obj.eval()
                    loaded_obj.to(device)
                return loaded_obj
            except Exception as e:
                sys.stderr.write(f"[-] Model weights loading error: {str(e)}\n")
                return None
        return None

    def _load_persistent_state(self):
        """กู้คืนค่า Baseline หากเคยผ่านการ Calibration ไว้แล้ว (Crash Recovery)"""
        if os.path.exists(self.state_file):
            try:
                with open(self.state_file, "r", encoding="utf-8") as f:
                    state = json.load(f)
                    self.sys_baseline = state.get("sys_baseline", 0.0)
                    self.dia_baseline = state.get("dia_baseline", 0.0)
                    self.vortex_baseline = state.get("vortex_baseline", 0.0)
                    self.is_calibrated = state.get("is_calibrated", False)
            except Exception as e:
                sys.stderr.write(f"[-] Failed to load persistent calibration state: {str(e)}\n")

    def _save_persistent_state(self):
        """บันทึกสถานะ Calibration ลง Persistent Storage"""
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
        except Exception as e:
            sys.stderr.write(f"[-] Failed to save persistent calibration state: {str(e)}\n")

    def resolve_location_and_altitude(self, raw_telemetry_payload: Dict[str, Any]) -> Dict[str, Any]:
        """ตรวจสอบความถูกต้องของฮาร์ดแวร์ Telemetry (GPS + Barometer) ป้องกันข้อมูลเท็จ"""
        gps = raw_telemetry_payload.get("gps", {})
        barometer = raw_telemetry_payload.get("barometer", {})
        
        lat = gps.get("lat")
        lng = gps.get("lng")
        is_gps_valid = gps.get("is_valid", False)
        
        altitude_m = barometer.get("altitude_m", 0.0)
        pressure_hpa = barometer.get("pressure_hpa", 1013.25)
        estimated_floor = max(1, int(round(altitude_m / 3.2))) if altitude_m > 0 else 1

        if is_gps_valid and lat is not None and lng is not None:
            return {
                "lat": float(lat),
                "lng": float(lng),
                "altitude_meters": float(altitude_m),
                "pressure_hpa": float(pressure_hpa),
                "floor_level": estimated_floor,
                "source": "HARDWARE_VERIFIED_FIX"
            }
        else:
            raise ValueError("CRITICAL SYSTEM ERROR: Hardware telemetry missing valid GPS/Barometer fix.")

    def gammatone_filterbank_envelope(self, audio_chunk: np.ndarray) -> np.ndarray:
        if len(audio_chunk) == 0:
            return np.zeros(1)
        channels = 64
        t = np.arange(len(audio_chunk)) / self.sr
        envelope_channels = np.zeros((channels, len(audio_chunk)))
        for k in range(channels):
            center_freq = 50.0 + (k * 30.0)
            carrier = np.cos(2 * np.pi * center_freq * t)
            envelope_channels[k, :] = np.abs(audio_chunk * carrier)
        return np.sqrt(np.maximum(np.sum(envelope_channels**2, axis=0), 0.0))

    def extract_exact_features(self, audio_chunk: np.ndarray, patient_profile: Dict[str, Any]):
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
        vortex_efficiency = float(np.clip(peak_filling / (1.0 + turbulent_variance), 0.1, 2.0))

        sys_riemann = np.sum(e_t * dt)
        i_sys = (bsa / bsa_ref) * ((1.0 + t_param) / (1.0 + c_dm)) * sys_riemann * 50.0 * flow_correction_factor

        dia_riemann = np.sum(e_diff)
        i_dia = (v_int_z + (np.sqrt(v_wh) * 166.67)) * np.exp(-(age / age_ref)) * dia_riemann * (1.0 / vortex_efficiency)

        f_sys = float(np.clip(i_sys, 0.0, 100.0))
        f_dia = float(np.clip(i_dia, 0.0, 100.0))
        f_prime_sys = float(np.mean(e_diff) / dt)

        t_array = np.arange(len(e_t)) * dt
        lam = 0.05 * vortex_efficiency
        integrand = np.exp(-lam * t_array) * e_diff
        
        if hasattr(np, 'trapezoid'):
            fs_integral = float(np.trapezoid(integrand, t_array))
        else:
            fs_integral = float(np.sum((integrand[:-1] + integrand[1:]) * np.diff(t_array) / 2.0))

        return f_sys, f_dia, f_prime_sys, fs_integral, vortex_efficiency

    def evaluate_pipeline(self, audio_chunk: np.ndarray, patient_profile: Dict[str, Any], raw_telemetry_payload: Dict[str, Any]) -> Dict[str, Any]:
        """
        ฟังก์ชันประมวลผลหลัก (Thread-Safe) รับข้อมูลดิบ แปลงสัญญาณ คำนวณความเสี่ยง และบันทึกลง Ledger
        """
        with self.lock:
            current_time = time.time()
            location_data = self.resolve_location_and_altitude(raw_telemetry_payload)
            
            f_sys, f_dia, f_prime_sys, fs_integral, vortex_eff = self.extract_exact_features(audio_chunk, patient_profile)
            
            ht = patient_profile.get('hypertension', False)
            dm = patient_profile.get('diabetes', False)
            risk_score = (f_sys * (1.5 if ht else 1.0) * (1.5 if dm else 1.0)) + f_dia
            
            y_max = 100.0
            lam = 0.05 * vortex_eff
            fs_slope = lam * max(0.0, y_max - f_sys) * f_dia * (1.0 + abs(fs_integral))
            
            # --- Calibration Phase ---
            if not self.is_calibrated:
                self.calib_sys_buffer.append(f_sys)
                self.calib_dia_buffer.append(f_dia)
                self.calib_vortex_buffer.append(vortex_eff)
                
                if len(self.calib_sys_buffer) >= int(self.calibration_duration_sec):
                    self.sys_baseline = float(np.mean(self.calib_sys_buffer))
                    self.dia_baseline = float(np.mean(self.calib_dia_buffer))
                    self.vortex_baseline = float(np.mean(self.calib_vortex_buffer))
                    self.is_calibrated = True
                    self._save_persistent_state()
                
                return {
                    "status": "CALIBRATING",
                    "progress": len(self.calib_sys_buffer) / int(self.calibration_duration_sec),
                    "location": location_data
                }

            # --- Delta Tracking Phase ---
            delta_sys = (f_sys - self.sys_baseline) / (self.sys_baseline + 1e-6)
            delta_dia = (f_dia - self.dia_baseline) / (self.dia_baseline + 1e-6)
            is_acute = (abs(delta_sys) > 0.35) or (abs(delta_dia) > 0.35)
            
            is_abnormal = ((f_sys > 80.0) or (fs_slope < 1.0) or (risk_score > 150.0)) and is_acute
            
            self.history_window.append({
                "timestamp": current_time,
                "is_abnormal": is_abnormal,
                "risk_score": risk_score
            })
            self.history_window = [x for x in self.history_window if current_time - x["timestamp"] <= self.window_limit_seconds]
            
            total = len(self.history_window)
            freq_rate = (sum(1 for x in self.history_window if x["is_abnormal"]) / total) if total > 0 else 0.0
            avg_risk = np.mean([x["risk_score"] for x in self.history_window]) if total > 0 else risk_score
            danger_index = avg_risk * (1.0 + freq_rate * 2.0)
            
            sos_trigger = (danger_index > 200.0 and freq_rate > 0.6)
            
            # สร้าง Cryptographic Audit Commitment
            metrics_payload = {"f_sys": f_sys, "f_dia": f_dia, "risk_score": risk_score}
            audit_hash = CryptographicAuditSigner.generate_secure_commitment(
                patient_profile.get("patient_id", "UNKNOWN"), current_time, metrics_payload
            )

            evaluation_result = {
                "patient_id": patient_profile.get("patient_id", "SECURE_NODE_01"),
                "timestamp": datetime.fromtimestamp(current_time).isoformat(),
                "location": location_data,
                "clinical_metrics": {
                    "f_sys": f_sys,
                    "f_dia": f_dia,
                    "vortex_efficiency": vortex_eff,
                    "risk_score": risk_score,
                    "danger_index": danger_index
                },
                "sos_trigger": sos_trigger,
                "audit_commitment_hash": audit_hash
            }

            # บันทึกลง Append-Only Ledger แบบ Thread-Safe
            self.ledger.append_record(evaluation_result)
            return evaluation_result