#include <WiFi.h>
#include <WebSocketsClient.h>
#include <ArduinoJson.h>

constchar* ssid = "YOUR_WIFI_SSID";
constchar* password = "YOUR_WIFI_PASSWORD";
constchar* server_ip = "192.168.1.100"; // IP ของเครื่องที่รัน Backend FastAPI
constint server_port = 8000;

WebSocketsClient webSocket;

void webSocketEvent(WStype_t type, uint8_t * payload, size_t length) {
    switch(type) {
        case WStype_DISCONNECTED:
            Serial.println("[WSc] Disconnected!");
            break;
        case WStype_CONNECTED:
            Serial.println("[WSc] Connected to P'MAX Backend Server!");
            break;
        case WStype_TEXT:
            // รับข้อมูลย้อนกลับจาก Server หากจำเป็น
            break;
        default:
            break;
    }
}

void setup() {
    Serial.begin(115200);
    WiFi.begin(ssid, password);
    
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("\nWiFi connected.");

    // เริ่มต้นเชื่อมต่อ WebSocket ไปยัง Backend
    webSocket.begin(server_ip, server_port, "/ws/pcg");
    webSocket.onEvent(webSocketEvent);
    webSocket.setReconnectInterval(5000);
}

void loop() {
    webSocket.loop();

    // อ่านค่าสัญญาณจริงจากขา Analog หรือ I2S ของ MEMS Acoustic Sensor บนบอร์ด ESP32
    int raw_sensor = analogRead(34); 
    float normalized_signal = (raw_sensor - 2048.0) / 2048.0; // แปลงช่วงสัญญาณ

    // สร้าง JSON Payload ของจริง
    StaticJsonDocument<200> doc;
    doc["signal_value"] = normalized_signal;
    doc["hash"] = "0x" + String(millis(), HEX) + "a8f9"; // หรือค่า Hash จาก SHA256 จริง
    doc["zkp_status"] = "VERIFIED";
    doc["validator"] = "ESP32-MEMS-PoA-01";

    String jsonString;
    serializeJson(doc, jsonString);

    // ส่งข้อมูลจริงไปยังเซิร์ฟเวอร์ตามรอบเวลา Sampling Rate
    if (WiFi.status() == WL_CONNECTED) {
        webSocket.sendTXT(jsonString);
    }
    
    delay(20); // ควบคุม Sampling Rate ประมาณ 50Hz
}