import React, { useState } from 'react';
import { StyleSheet, Text, View, TouchableOpacity, ScrollView, SafeAreaView, ActivityIndicator } from 'react-native';

export default function App() {
  const [loading, setLoading] = useState(false);
  const [resultData, setResultData] = useState(null);

  // ฟจำลองการรับค่าจากอุปกรณ์ Hardware IoT Stethoscope & BP Monitor
  const handleSimulateHardwareData = async () => {
    setLoading(true);
    
    const payload = {
      patient_id: "PATIENT-TH-9921",
      systolic: 142,
      diastolic: 92,
      heart_rate: 88,
      sound_classification: "ABNORMAL_MURMUR", // ตรวจพบความผิดปกติ
      s1_timestamp_ms: 120.5,                  // เวลาที่พบ S1
      s2_timestamp_ms: 450.2,                  // เวลาที่พบ S2
      preload_mmHg: 14.5,                      // ปริมาตร/แรงดันเลือดก่อนบีบ
      afterload_dynes: 1250.0,                 // แรงต้านหลอดเลือด
      frank_starling_slope: 2.15,              // ความชันการบีบตัวของหัวใจ
      demographics: {
        age: 45,
        gender: "Male",
        weight_kg: 78.5,
        height_cm: 172.0,
        underlying_conditions: ["Hypertension"]
      }
    };

    try {
      // เปลี่ยน IP เป็น IP ของเครื่องคอมพิวเตอร์ที่คุณรัน FastAPI (เช่น http://192.168.1.50:8000/api/v1/medical/record)
      const response = await fetch('http://localhost:8000/api/v1/medical/record', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify(payload)
      });
      const data = await response.json();
      setResultData(data);
    } catch (error) {
      alert('Connection Error: ไม่สามารถเชื่อมต่อกับ Backend ได้');
    } finally {
      setLoading(false);
    }
  };

  return (
    <SafeAreaView style={styles.container}>
      <ScrollView contentContainerStyle={styles.scroll}>
        
        {/* Header Section */}
        <View style={styles.header}>
          <Text style={styles.brand}>VITALPULSE ZKP</Text>
          <Text style={styles.subtitle}>Cardiovascular IoT & Clinical Gateway</Text>
        </View>

        {/* Real-time Vitals & Hardware Card */}
        <View style={styles.card}>
          <Text style={styles.cardTitle}>🫀 Live Hardware Diagnostics</Text>
          
          <View style={styles.row}>
            <View>
              <Text style={styles.label}>BP (SYSTOLIC / DIASTOLIC)</Text>
              <Text style={styles.value}>142 / 92 mmHg</Text>
            </View>
            <View>
              <Text style={styles.label}>HEART RATE</Text>
              <Text style={styles.value}>88 BPM</Text>
            </View>
          </View>

          <View style={styles.divider} />

          {/* ข้อมูลจำเพาะเสียงหัวใจและเวลา S1/S2 */}
          <Text style={styles.label}>ACOUSTIC TIMING & HEMODYNAMICS</Text>
          <View style={styles.subInfoBox}>
            <Text style={styles.subText}>• Status: <Text style={{color: '#8B0000', fontWeight: 'bold'}}>ABNORMAL (Murmur)</Text></Text>
            <Text style={styles.subText}>• S1 Timestamp: 120.5 ms</Text>
            <Text style={styles.subText}>• S2 Timestamp: 450.2 ms</Text>
            <Text style={styles.subText}>• Preload: 14.5 mmHg | Afterload: 1250 dynes</Text>
            <Text style={styles.subText}>• Frank-Starling Slope: 2.15 (Contractility)</Text>
          </View>
        </View>

        {/* Action Trigger Card */}
        <View style={styles.actionCard}>
          <Text style={styles.actionTitle}>Zero-Knowledge Proof & Ledger</Text>
          <Text style={styles.actionDesc}>
            ส่งข้อมูลผ่านระบบเข้ารหัส ZKP เพื่อยืนยันความผิดปกติโดยไม่เปิดเผยตัวตนผู้ป่วยสู่สาธารณะ
          </Text>
          
          <TouchableOpacity style={styles.button} onPress={handleSimulateHardwareData} disabled={loading}>
            {loading ? (
              <ActivityIndicator color="#FFFFFF" />
            ) : (
              <Text style={styles.buttonText}>⚡ Process & Submit to Blockchain</Text>
            )}
          </TouchableOpacity>
        </View>

        {/* Result Display Box */}
        {resultData && (
          <View style={styles.resultCard}>
            <Text style={styles.resultTitle}>✅ Verified Block Output</Text>
            <Text style={styles.resultText}><Text style={{fontWeight: '600'}}>Tx Hash:</Text> {resultData.transaction_id}</Text>
            <Text style={styles.resultText}><Text style={{fontWeight: '600'}}>Block Height:</Text> #{resultData.block_height}</Text>
            <Text style={styles.resultText}><Text style={{fontWeight: '600'}}>Clinical Insight:</Text> {resultData.analysis.clinical_insight}</Text>
          </View>
        )}

      </ScrollView>
    </SafeAreaView>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#FAFAFC' },
  scroll: { padding: 20 },
  header: { marginTop: 20, marginBottom: 20 },
  brand: { fontSize: 18, fontWeight: '700', color: '#1A1A1A', letterSpacing: 1.5 },
  subtitle: { fontSize: 13, color: '#666', marginTop: 4 },
  card: { backgroundColor: '#FFFFFF', borderRadius: 12, padding: 20, marginBottom: 20, borderWidth: 1, borderColor: '#E5E5EA' },
  cardTitle: { fontSize: 16, fontWeight: '600', color: '#1A1A1A', marginBottom: 15 },
  row: { flexDirection: 'row', justifyContent: 'space-between', marginBottom: 15 },
  label: { fontSize: 11, color: '#8E8E93', fontWeight: '600', letterSpacing: 0.5 },
  value: { fontSize: 20, fontWeight: '600', color: '#1A1A1A', marginTop: 4 },
  divider: { height: 1, backgroundColor: '#E5E5EA', marginVertical: 15 },
  subInfoBox: { marginTop: 8, backgroundColor: '#F4F4F6', padding: 12, borderRadius: 8 },
  subText: { fontSize: 13, color: '#333', marginBottom: 4 },
  actionCard: { backgroundColor: '#8B0000', borderRadius: 12, padding: 20, marginBottom: 20 },
  actionTitle: { fontSize: 18, fontWeight: '600', color: '#FFFFFF', marginBottom: 8 },
  actionDesc: { fontSize: 13, color: '#FFD1D1', lineHeight: 18, marginBottom: 20 },
  button: { backgroundColor: '#1A1A1A', borderRadius: 8, padding: 14, alignItems: 'center' },
  buttonText: { color: '#FFFFFF', fontWeight: '600', fontSize: 14 },
  resultCard: { backgroundColor: '#FFFFFF', borderRadius: 12, padding: 20, marginBottom: 20, borderWidth: 1, borderColor: '#8B0000' },
  resultTitle: { fontSize: 16, fontWeight: '600', color: '#8B0000', marginBottom: 10 },
  resultText: { fontSize: 13, color: '#333', marginBottom: 6, lineHeight: 18 }
});