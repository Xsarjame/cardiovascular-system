import hashlib
import time
import json
import os
import sys
import numpy as np
from typing import List, Dict, Any, Optional
from pydantic import BaseModel
from fastapi import FastAPI, WebSocket, WebSocketDisconnect, HTTPException, status, UploadFile, File, Form
from fastapi.middleware.cors import CORSMiddleware
from fastapi.responses import HTMLResponse, FileResponse
from fastapi.staticfiles import StaticFiles
from io import BytesIO
import scipy.io.wavfile as wavfile
from datetime import datetime

# เพิ่ม Path สำหรับดึง Module จากโฟลเดอร์เดียวกัน
sys.path.append(os.path.abspath(os.path.dirname(__file__)))
from pmax_production_core import PMaxProductionCore

app = FastAPI(title="P'MAX Production Backend & ZKP Blockchain", version="2.0.0")

app.add_middleware(
    CORSMiddleware,
    allow_origins=["*"],
    allow_credentials=True,
    allow_methods=["*"],
    allow_headers=["*"],
)

# Initialize P'Max Production Core Engine จริง
engine = PMaxProductionCore()

# ระบบสแกนหาโฟลเดอร์ที่มีไฟล์ HTML ทั่วทั้งโปรเจกต์โดยอัตโนมัติ
BASE_DIR = os.path.dirname(os.path.abspath(__file__)) # โฟลเดอร์ Cardiac_Backend
ROOT_DIR = os.path.dirname(BASE_DIR)                 # โฟลเดอร์หลักของโปรเจกต์

def find_file_recursive(start_dir, filename):
    for root, dirs, files in os.walk(start_dir):
        if filename in files:
            return os.path.join(root, filename)
    return None

# ค้นหาไฟล์ dashboard.html และ blockchain.html ทั่วโปรเจกต์
DASHBOARD_PATH = find_file_recursive(ROOT_DIR, "dashboard.html")
BLOCKCHAIN_PATH = find_file_recursive(ROOT_DIR, "blockchain.html")

print(f"[+] Found dashboard.html at: {DASHBOARD_PATH}")
print(f"[+] Found blockchain.html at: {BLOCKCHAIN_PATH}")

# กำหนดพาทกลางสำหรับ Static Files (ใช้โฟลเดอร์ที่เจอไฟล์ HTML)
STATIC_DIR = os.path.dirname(DASHBOARD_PATH) if DASHBOARD_PATH else BASE_DIR
if os.path.exists(STATIC_DIR):
    app.mount("/static", StaticFiles(directory=STATIC_DIR), name="static")


# ==========================================
# 0. Root & HTML Views Endpoints (Auto-Find)
# ==========================================
@app.get("/", response_class=HTMLResponse)
def read_root():
    global DASHBOARD_PATH
    if not DASHBOARD_PATH or not os.path.exists(DASHBOARD_PATH):
        DASHBOARD_PATH = find_file_recursive(ROOT_DIR, "dashboard.html")
        
    if DASHBOARD_PATH and os.path.exists(DASHBOARD_PATH):
        return FileResponse(DASHBOARD_PATH)
    
    return {
        "status": "Online",
        "system": "P'MAX Production Backend & ZKP Blockchain",
        "version": "2.0.0",
        "error": "dashboard.html could not be found anywhere in the project directory.",
        "documentation": "/docs"
    }

@app.get("/dashboard.html", response_class=HTMLResponse)
def get_dashboard_page():
    global DASHBOARD_PATH
    if not DASHBOARD_PATH or not os.path.exists(DASHBOARD_PATH):
        DASHBOARD_PATH = find_file_recursive(ROOT_DIR, "dashboard.html")
        
    if DASHBOARD_PATH and os.path.exists(DASHBOARD_PATH):
        return FileResponse(DASHBOARD_PATH)
    raise HTTPException(status_code=404, detail="dashboard.html not found in project directory.")

@app.get("/blockchain.html", response_class=HTMLResponse)
def get_blockchain_page():
    global BLOCKCHAIN_PATH
    if not BLOCKCHAIN_PATH or not os.path.exists(BLOCKCHAIN_PATH):
        BLOCKCHAIN_PATH = find_file_recursive(ROOT_DIR, "blockchain.html")
        
    if BLOCKCHAIN_PATH and os.path.exists(BLOCKCHAIN_PATH):
        return FileResponse(BLOCKCHAIN_PATH)
    raise HTTPException(status_code=404, detail="blockchain.html not found in project directory.")


# ==========================================
# 1. โครงสร้าง Block และ Blockchain พร้อม Serializer ป้องกัน Type Error
# ==========================================
class Block:
    def __init__(self, index: int, timestamp: float, data: Dict[str, Any], previous_hash: str):
        self.index = index
        self.timestamp = timestamp
        self.data = data  
        self.previous_hash = previous_hash
        self.nonce = 0
        self.hash = self.calculate_hash()

    def calculate_hash(self) -> str:
        def default_serializer(obj):
            if isinstance(obj, (np.float32, np.float64, np.floating)):
                return float(obj)
            if isinstance(obj, (np.int32, np.int64, np.integer)):
                return int(obj)
            if isinstance(obj, np.ndarray):
                return obj.tolist()
            raise TypeError(f"Object of type {type(obj)} is not JSON serializable")

        block_string = json.dumps({
            "index": int(self.index),
            "timestamp": float(self.timestamp),
            "data": self.data,
            "previous_hash": str(self.previous_hash),
            "nonce": int(self.nonce)
        }, sort_keys=True, default=default_serializer).encode()
        
        return hashlib.sha256(block_string).hexdigest()

    def mine_block(self, difficulty: int = 2):
        target = "0" * difficulty
        while self.hash[:difficulty] != target:
            self.nonce += 1
            self.hash = self.calculate_hash()


class MedicalBlockchain:
    def __init__(self):
        self.chain: List[Block] = [self.create_genesis_block()]
        self.difficulty = 2  

    def create_genesis_block(self) -> Block:
        return Block(0, time.time(), {"message": "Genesis Block - Medical ZKP Gateway"}, "0")

    def get_latest_block(self) -> Block:
        return self.chain[-1]

    def add_block(self, clinical_data: Dict[str, Any]) -> Block:
        latest_block = self.get_latest_block()
        new_block = Block(
            index=len(self.chain),
            timestamp=time.time(),
            data=clinical_data,
            previous_hash=latest_block.hash
        )
        new_block.mine_block(self.difficulty)
        self.chain.append(new_block)
        return new_block

    def is_chain_valid(self) -> bool:
        for i in range(1, len(self.chain)):
            current_block = self.chain[i]
            previous_block = self.chain[i-1]

            if current_block.hash != current_block.calculate_hash():
                return False
            
            if current_block.previous_hash != previous_block.hash:
                return False
        return True


medical_blockchain = MedicalBlockchain()


# ==========================================
# 2. จัดการการเชื่อมต่อ WebSocket สำหรับ ESP32
# ==========================================
class ConnectionManager:
    def __init__(self):
        self.active_connections: List[WebSocket] = []

    async def connect(self, websocket: WebSocket):
        await websocket.accept()
        self.active_connections.append(websocket)

    def disconnect(self, websocket: WebSocket):
        self.active_connections.remove(websocket)

    async def broadcast(self, message: str):
        for connection in self.active_connections:
            await connection.send_text(message)

manager = ConnectionManager()


# ==========================================
# 3. FastAPI Endpoints 
# ==========================================

class RecordResultRequest(BaseModel):
    patient_id: str
    f_sys: float
    f_dia: float
    risk_score: float
    risk_tier: str
    zkp_proof_hash: str = ""  


@app.post("/api/v1/blockchain/record")
def record_to_blockchain(request: RecordResultRequest):
    payload = {
        "patient_id": request.patient_id,
        "f_sys": request.f_sys,
        "f_dia": request.f_dia,
        "risk_score": request.risk_score,
        "risk_tier": request.risk_tier,
        "zkp_proof_hash": request.zkp_proof_hash or hashlib.sha256(request.patient_id.encode()).hexdigest()
    }
    
    new_block = medical_blockchain.add_block(payload)
    
    return {
        "status": "Success",
        "message": "Clinical data securely recorded on Medical Blockchain",
        "block_index": new_block.index,
        "block_hash": new_block.hash,
        "previous_hash": new_block.previous_hash
    }


@app.get("/api/v1/blockchain/chain")
def get_blockchain():
    chain_data = []
    for block in medical_blockchain.chain:
        chain_data.append({
            "index": block.index,
            "timestamp": block.timestamp,
            "data": block.data,
            "hash": block.hash,
            "previous_hash": block.previous_hash
        })
    
    return {
        "is_valid": medical_blockchain.is_chain_valid(),
        "chain_length": len(chain_data),
        "chain": chain_data
    }


# ==========================================
# 4. ส่วนประมวลผลเสียงและ Real-time Endpoints
# ==========================================

def process_uploaded_wav(file_bytes: bytes, target_sr: int = 2000) -> np.ndarray:
    try:
        sr, audio_data = wavfile.read(BytesIO(file_bytes))
        if len(audio_data.shape) > 1:
            audio_data = np.mean(audio_data, axis=1)
        audio_data = audio_data.astype(np.float32)
        max_val = np.max(np.abs(audio_data))
        if max_val > 0:
            audio_data /= max_val

        if sr != target_sr and len(audio_data) > 0:
            duration = len(audio_data) / sr
            target_length = int(duration * target_sr)
            audio_data = np.interp(
                np.linspace(0, len(audio_data) - 1, target_length),
                np.arange(len(audio_data)),
                audio_data
            )
        return audio_data
    except Exception as e:
        raise HTTPException(status_code=400, detail=f"Invalid WAV format: {str(e)}")


@app.post("/api/v1/detect-cardiac-real")
async def detect_cardiac_real_data(
    patient_id: str = Form(...),
    age: int = Form(...),
    bsa: float = Form(1.73),
    hypertension: bool = Form(False),
    diabetes: bool = Form(False),
    telemetry_json: str = Form(...),
    audio_file: UploadFile = File(...)
):
    audio_bytes = await audio_file.read()
    audio_chunk = process_uploaded_wav(audio_bytes, target_sr=2000)
    
    try:
        raw_telemetry = json.loads(telemetry_json)
    except json.JSONDecodeError:
        raise HTTPException(status_code=400, detail="Malformed telemetry_json payload.")

    patient_profile = {
        "patient_id": patient_id,
        "age": age,
        "bsa": bsa,
        "hypertension": hypertension,
        "diabetes": diabetes
    }

    f_sys, f_dia, f_prime_sys, fs_integral, vortex_eff = engine.extract_exact_features(audio_chunk, patient_profile)

    recorded_time_str = datetime.now().strftime('%H:%M:%S')
    gps = raw_telemetry.get("gps", {})
    barometer = raw_telemetry.get("barometer", {})
    location_summary = f"Lat: {gps.get('lat', 'N/A')}, Lng: {gps.get('lng', 'N/A')}, Alt: {barometer.get('altitude_m', 'N/A')}m"

    evaluation_result = engine.evaluate_frank_starling_system(
        f_sys=f_sys,
        f_dia=f_dia,
        fs_integral=fs_integral,
        vortex_efficiency=vortex_eff,
        patient_profile=patient_profile,
        recorded_time=recorded_time_str,
        location=location_summary
    )

    clinical_payload = {
        "patient_id": patient_id,
        "f_sys": evaluation_result.get("F_sys", 0.0),
        "f_dia": evaluation_result.get("F_dia", 0.0),
        "risk_score": evaluation_result.get("Risk_Score", 0.0),
        "risk_tier": evaluation_result.get("Risk_Tier", "Normal"),
        "zkp_proof_hash": hashlib.sha256(f"{patient_id}:{evaluation_result.get('Risk_Score', 0.0)}".encode()).hexdigest(),
        "location": location_summary,
        "sos_trigger": evaluation_result.get("SOS_Trigger", False)
    }
    
    new_block = medical_blockchain.add_block(clinical_payload)

    return {
        "status": "Success",
        "evaluation": evaluation_result,
        "blockchain_record": {
            "block_index": new_block.index,
            "block_hash": new_block.hash,
            "previous_hash": new_block.previous_hash
        }
    }


@app.websocket("/ws/pcg")
async def websocket_esp32_endpoint(websocket: WebSocket):
    await manager.connect(websocket)
    try:
        while True:
            raw_data = await websocket.receive_text()
            packet = json.loads(raw_data)
            
            clinical_payload = {
                "patient_id": packet.get("patient_id", "ESP32-NODE-01"),
                "signal_value": packet.get("signal_value", 0.0),
                "zkp_status": packet.get("zkp_status", "VERIFIED"),
                "device_hash": packet.get("hash", "")
            }
            
            medical_blockchain.add_block(clinical_payload)
            await manager.broadcast(json.dumps(packet))
            
    except WebSocketDisconnect:
        manager.disconnect(websocket)


# ==========================================
# 5. Uvicorn Server Runner
# ==========================================
import os
import uvicorn

if __name__ == "__main__":
    port = int(os.environ.get("PORT", 8000))
    uvicorn.run("main:app", host="0.0.0.0", port=port)
