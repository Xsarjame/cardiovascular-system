import os
import numpy as np
import matplotlib.pyplot as plt
import matplotlib.animation as animation

class CardiacVisualizer:
    def __init__(self, save_dir="./output_plots"):
        self.save_dir = save_dir
        os.makedirs(self.save_dir, exist_ok=True)

    def plot_frank_starling_snapshot(self, history_data, current_patient_id="Unknown"):
        """
        พลอต Frank-Starling Diagram จากประวัติย้อนหลัง
        แกน X: Preload (F_dia)
        แกน Y: Stroke Volume / Active Tension (F_sys)
        """
        plt.figure(figsize=(8, 6))
        
        # แยกข้อมูลปกติและความผิดปกติเพื่อแยกสีให้ชัดเจน
        normal_x = [d["F_dia"] for d in history_data if not d["is_abnormal"]]
        normal_y = [d["F_sys"] for d in history_data if not d["is_abnormal"]]
        
        abnormal_x = [d["F_dia"] for d in history_data if d["is_abnormal"]]
        abnormal_y = [d["F_sys"] for d in history_data if d["is_abnormal"]]

        # พลอตจุดปกติ (สีเขียว/น้ำเงิน)
        plt.scatter(normal_x, normal_y, color='blue', alpha=0.6, label='Normal Range', s=40)
        
        # พลอตจุดผิดปกติ (สีแดง - เน้นย้ำ)
        if abnormal_x:
            plt.scatter(abnormal_x, abnormal_y, color='red', alpha=0.9, label='Abnormal / Risk Triggered', s=70, marker='X')

        # วาดเส้นมาตรฐานอ้างอิง Frank-Starling Law Curve (สมการจำลองแนวโน้ม)
        x_curve = np.linspace(0, 100, 100)
        y_curve = 100 * (1 - np.exp(-0.05 * x_curve)) # เส้นโค้งทางสรีรวิทยา
        plt.plot(x_curve, y_curve, color='gray', linestyle='--', label='Reference Curve')

        plt.title(f"Frank-Starling Real-time Diagnostic - Patient: {current_patient_id}")
        plt.xlabel("Preload Indicator ($F_{dia}$)")
        plt.ylabel("Stroke Volume / Active Tension ($F_{sys}$)")
        plt.xlim(0, 100)
        plt.ylim(0, 100)
        plt.legend(loc='upper left')
        plt.grid(True, linestyle=':', alpha=0.6)

        # บันทึกไฟล์ภาพเก็บไว้หรือแสดงผล
        file_path = os.path.join(self.save_dir, f"frank_starling_{current_patient_id}.png")
        plt.savefig(file_path, dpi=300, bbox_inches='tight')
        plt.close()
        print(f"📊 [Visualizer] บันทึกกราฟ Frank-Starling สำเร็จที่: {file_path}")
        return file_path

    def plot_pressure_volume_loop_conditional(self, f_sys, f_dia, is_abnormal, current_patient_id="Unknown"):
        """
        พลอต Pressure-Volume (PV) Loop เฉพาะเมื่อตรวจพบความผิดปกติเท่านั้น (Event-triggered)
        ตามหลักการเปลี่ยนแปลงปริมาตรและความดันในห้องหัวใจห้องล่างซ้าย (LV Loop)
        """
        if not is_abnormal:
            # ถ้าปกติจะไม่ทำการพลอตและสิ้นเปลืองทรัพยากร ตามเงื่อนไขที่คุณต้องการ
            return None

        plt.figure(figsize=(7, 7))

        # จำลองรูปทรง PV Loop มาตรฐานทางสรีรวิทยา (ปรับสเกลตามค่า F_sys และ F_dia ที่คำนวณได้)
        # 1. Isovolumetric Contraction -> 2. Ejection -> 3. Isovolumetric Relaxation -> 4. Filling
        v_min, v_max = 50.0, 120.0 + (f_dia * 0.5)
        p_min, p_max = 10.0, 80.0 + (f_sys * 1.2)

        # จุดมุมของ PV Loop
        volumes = [v_min, v_min, v_max, v_max, v_min]
        pressures = [p_min, p_max, p_max, p_min, p_min]

        # วาดลูป PV
        plt.plot(volumes, pressures, color='crimson', linewidth=2.5, label='Abnormal PV Loop (Triggered)')
        plt.fill(volumes, pressures, color='crimson', alpha=0.15)

        plt.title(f"🚨 PV Loop Alert: Abnormal Hemodynamics Detected!\nPatient: {current_patient_id}", color='darkred', fontweight='bold')
        plt.ylabel("Left Ventricular Pressure (mmHg)")
        plt.xlabel("Left Ventricular Volume (mL)")
        plt.grid(True, linestyle='--', alpha=0.5)
        plt.legend(loc='upper right')

        # บันทึกภาพเฉพาะเคสผิดปกติ
        file_path = os.path.join(self.save_dir, f"pv_loop_alert_{current_patient_id}_{int(time.time())}.png")
        plt.savefig(file_path, dpi=300, bbox_inches='tight')
        plt.close()
        
        print(f"🚨 [Visualizer - Triggered] ตรวจพบความผิดปกติ! บันทึก PV Loop แหลมคมที่: {file_path}")
        return file_path