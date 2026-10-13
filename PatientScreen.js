import React, { useState } from 'react';
import { StyleSheet, Text, View, TouchableOpacity, ScrollView, SafeAreaView, ActivityIndicator, Alert } from 'react-native';

export default function PatientScreen() {
  const [loading, setLoading] = useState(false);
  const [sosActive, setSosActive] = useState(false);
  const [resultData, setResultData] = useState(null);

  // ฟังก์ชันจำลองการส่งข้อมูลและประเมินความเสี่ยงฉุกเฉิน (รวมถึงกรณีเสี่ยงหัวใจหยุดเต้น)
  const handleEmergencyCheck = async () => {
    setLoading(true);
    
    const emergencyPayload = {
      patient_id: "PATIENT-TH-9921",
      demographics: {
        name: "คุณสมชาย ใจดี",
        age: 58,
        gender: "Male",
        weight_kg: 75.0,
        height_cm: 168.0,
        underlying_conditions: ["Coronary Artery Disease", "Hypertension"],
        emergency_contact: {
          guardian_name: "สุดา ใจดี (ภรรยา)",
          phone: "089-123-4567"
        }
      },
      vitals: {
        systolic: 180, // ค่าความดันสูงผิดปกติ
        diastolic: 110,
        heart_rate: 135,
        sound_classification: "ABNORMAL_CRITICAL_MURMUR",
        s1_timestamp_ms: 95.0,
        s2_timestamp_ms: 310.4,
        preload_mmHg: 22.1,
        afterload_dynes: 1650.0,
        frank_starling_slope: 1.05 // บ่งชี้ภาวะหัวใจล้มเหลว/เสี่ยงวิกฤต
      }
    };

    try {
      // จำลองการยิง API ไปยัง Backend เพื่อประเมินความเสี่ยงและสั่งการ SOS
      setTimeout(() => {
        setLoading(false);
        setSosActive(true);
        setResultData({
          status: "CRITICAL_ALERT",
          message: "AI ตรวจพบความเสี่ยงภาวะหัวใจหยุดเต้นสูง! ระบบกำลังติดต่อโรงพยาบาลและแจ้งเตือนญาติ",
          hospital_notified: "โรงพยาบาลจุฬาลงกรณ์ (ระยะทาง 1.2 กม.)",
          guardian_alerted: emergencyPayload.demographics.emergency_contact.guardian_name,
          location: "Lat: 13.7367, Lng: 100.5331 (187 Rama IV Rd)",
          tx_hash: "0xemergency_tx_" + Math.random().toString(36).substring(7)
        });
        Alert.alert("🚨 ภาวะฉุกเฉิน!", "ระบบตรวจพบความเสี่ยงวิกฤต และได้ทำการส่งพิกัดแจ้งเตือนโรงพยาบาลและญาติเรียบร้อยแล้ว");
      }, 1500);

    } catch (error) {
      setLoading(false);
      alert('เกิดข้อผิดพลาดในการเชื่อมต่อระบบฉุกเฉิน');
    }
  };

  return (
    <SafeAreaView style={styles.container}>
      <ScrollView contentContainerStyle={styles.scroll}>
        
        {/* Header */}
        <View style={styles.header}>
          <Text style={styles.brand}>VITALPULSE ZKP</Text>
          <Text style={styles.subtitle}>Patient Safety & Emergency Gateway</Text>
        </View>

        {/* Patient & Guardian Profile Card */}
        <View style={styles.card}>
          <Text style={styles.cardTitle}>👤 Patient & Guardian Profile</Text>
          <Text style={styles.profileText}>• ผู้ป่วย: คุณสมชาย ใจดี (อายุ 58 ปี)</Text>
          <Text style={styles.profileText}>• โรคประจำตัว: ความดันโลหิตสูง, โรคหลอดเลือดหัวใจ</Text>
          <Text style={styles.profileText}><Text style={{fontWeight: '700', color: '#8B0000'}}>• ผู้ติดต่อฉุกเฉิน (ญาติ):</Text> สุดา ใจดี (โทร: 089-123-4567)</Text>
        </View>

        {/* Live Vitals & Hardware Card */}
        <View style={styles.card}>
          <Text style={styles.cardTitle}>🫀 Live IoT Vitals & AI Monitoring</Text>
          
          <View style={styles.row}>
            <View>
              <Text style={styles.label}>BP (SYSTOLIC / DIASTOLIC)</Text>
              <Text style={styles.valueWarning}>180 / 110 mmHg</Text>
            </View>
            <View>
              <Text style={styles.label}>HEART RATE</Text>
              <Text style={styles.valueWarning}>135 BPM</Text>
            </View>
          </View>

          <View style={styles.divider} />
          
          <View style={styles.subInfoBox}>
            <Text style={styles.subText}>• AI Status: <Text style={{color: '#8B0000', fontWeight: 'bold'}}>HIGH RISK (Cardiac Arrest Warning)</Text></Text>
            <Text style={styles.subText}>• Frank-Starling Slope: 1.05 (Contractility Drop)</Text>
          </View>
        </View>

        {/* Emergency SOS Trigger Card */}
        <View style={[styles.actionCard, sosActive ? styles.sosTriggeredCard : {}]}>
          <Text style={styles.actionTitle}>🚨 Emergency Automated SOS</Text>
          <Text style={styles.actionDesc}>
            ระบบ AI จะทำการโทรออกโรงพยาบาลและส่งตำแหน่งพิกัดพร้อมประวัติย่อให้ญาติอัตโนมัติเมื่อตรวจพบความผิดปกติวิกฤต
          </Text>
          
          <TouchableOpacity style={styles.sosButton} onPress={handleEmergencyCheck} disabled={loading}>
            {loading ? (
              <ActivityIndicator color="#FFFFFF" />
            ) : (
              <Text style={styles.sosButtonText}>⚠️ จำลองเหตุการณ์ฉุกเฉิน (Test SOS)</Text>
            )}
          </TouchableOpacity>
        </View>

        {/* Emergency Result Panel */}
        {resultData && (
          <View style={styles.resultCard}>
            <Text style={styles.resultTitle}>⚡ พิกัดและการแจ้งเตือนฉุกเฉินถูกส่งแล้ว</Text>
            <Text style={styles.resultText}><Text style={{fontWeight: '600'}}>สถานะ:</Text> {resultData.message}</Text>
            <Text style={styles.resultText}><Text style={{fontWeight: '600'}}>โรงพยาบาล:</Text> {resultData.hospital_notified}</Text>
            <Text style={styles.resultText}><Text style={{fontWeight: '600'}}>แจ้งเตือนญาติ:</Text> {resultData.guardian_alerted}</Text>
            <Text style={styles.resultText}><Text style={{fontWeight: '600'}}>พิกัด GPS:</Text> {resultData.location}</Text>
            <Text style={styles.resultText}><Text style={{fontWeight: '600'}}>Block Ledger Tx:</Text> {resultData.tx_hash}</Text>
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
  cardTitle: { fontSize: 16, fontWeight: '600', color: '#1A1A1A', marginBottom: 12 },
  profileText: { fontSize: 13, color: '#333', marginBottom: 6 },
  row: { flexDirection: 'row', justifyContent: 'space-between', marginBottom: 10 },
  label: { fontSize: 11, color: '#8E8E93', fontWeight: '600', letterSpacing: 0.5 },
  valueWarning: { fontSize: 20, fontWeight: '700', color: '#8B0000', marginTop: 4 },
  divider: { height: 1, backgroundColor: '#E5E5EA', marginVertical: 12 },
  subInfoBox: { backgroundColor: '#FFF5F5', padding: 12, borderRadius: 8, borderWidth: 1, borderColor: '#FEB2B2' },
  subText: { fontSize: 13, color: '#C53030', marginBottom: 4 },
  actionCard: { backgroundColor: '#8B0000', borderRadius: 12, padding: 20, marginBottom: 20 },
  sosTriggeredCard: { backgroundColor: '#742A2A', borderWidth: 2, borderColor: '#FEB2B2' },
  actionTitle: { fontSize: 18, fontWeight: '600', color: '#FFFFFF', marginBottom: 8 },
  actionDesc: { fontSize: 13, color: '#FFD1D1', lineHeight: 18, marginBottom: 20 },
  sosButton: { backgroundColor: '#1A1A1A', borderRadius: 8, padding: 14, alignItems: 'center' },
  sosButtonText: { color: '#FFFFFF', fontWeight: '700', fontSize: 14 },
  resultCard: { backgroundColor: '#FFFFFF', borderRadius: 12, padding: 20, marginBottom: 20, borderWidth: 2, borderColor: '#8B0000' },
  resultTitle: { fontSize: 16, fontWeight: '700', color: '#8B0000', marginBottom: 10 },
  resultText: { fontSize: 13, color: '#333', marginBottom: 6, lineHeight: 18 }
});