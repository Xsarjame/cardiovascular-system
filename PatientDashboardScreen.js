// screens/PatientDashboardScreen.js
import React, { useState } from 'react';
import { StyleSheet, Text, View, ScrollView, TouchableOpacity, Dimensions, TextInput } from 'react-native';

const screenWidth = Dimensions.get('window').width;

export default function PatientDashboardScreen({ userSession, onLogout }) {
  const [searchQuery, setSearchQuery] = useState('');
  const [proofStatus, setProofStatus] = useState('verified'); // verified, pending

  return (
    <View style={styles.container}>
      {/* Header */}
      <View style={styles.header}>
        <View>
          <Text style={styles.appTitle}>VITALPULSE ZKP</Text>
          <Text style={styles.appSubtitle}>● Secure Node Connected (ID: {userSession?.patient_id || 'PATIENT-01'})</Text>
        </View>
        <TouchableOpacity style={styles.logoutBtn} onPress={onLogout}>
          <Text style={styles.logoutText}>Logout</Text>
        </TouchableOpacity>
      </View>

      <ScrollView contentContainerStyle={styles.scrollContent} showsVerticalScrollIndicator={false}>
        
        {/* Network Metrics Overview */}
        <View style={styles.metricsRow}>
          <View style={styles.metricCard}>
            <Text style={styles.metricLabel}>Active Proofs</Text>
            <Text style={styles.metricValue}>12 📈</Text>
          </View>
          <View style={styles.metricCard}>
            <Text style={styles.metricLabel}>Avg. Latency</Text>
            <Text style={styles.metricValue}>142ms ⚡</Text>
          </View>
          <View style={styles.metricCard}>
            <Text style={styles.metricLabel}>Network Health</Text>
            <Text style={styles.metricValue}>99.9% 🛡️</Text>
          </View>
        </View>

        {/* Search Bar for Verification Queue */}
        <View style={styles.searchContainer}>
          <TextInput 
            style={styles.searchInput}
            placeholder="Search Patient ID or Proof Hash..."
            placeholderTextColor="#666"
            value={searchQuery}
            onChangeText={setSearchQuery}
          />
        </View>

        {/* Daily Trend Analytics Section (Mock Graphic / Chart Container) */}
        <View style={styles.card}>
          <View style={styles.cardHeader}>
            <Text style={styles.cardTitle}>Cardiovascular Trends (7 Days)</Text>
            <Text style={styles.badgeText}>Real-time BLE</Text>
          </View>
          <View style={styles.chartMockContainer}>
            <View style={styles.chartLineMock}>
              <Text style={styles.chartTooltip}>Systolic: 118 mmHg (Stable)</Text>
            </View>
            <View style={styles.chartAxis}>
              <Text style={styles.axisText}>Mon</Text>
              <Text style={styles.axisText}>Tue</Text>
              <Text style={styles.axisText}>Wed</Text>
              <Text style={styles.axisText}>Thu</Text>
              <Text style={styles.axisText}>Fri</Text>
              <Text style={styles.axisText}>Sat</Text>
              <Text style={styles.axisText}>Sun</Text>
            </View>
          </View>
        </View>

        {/* Verification Queue Item 1 */}
        <View style={styles.card}>
          <View style={styles.patientRow}>
            <View>
              <Text style={styles.patientName}>Elena Rodriguez</Text>
              <Text style={styles.proofHash}>Proof ID: 0x8f2...a1e</Text>
            </View>
            <View style={[styles.statusBadge, { backgroundColor: '#1C3A27' }]}>
              <Text style={[styles.statusText, { color: '#34C759' }]}>verified</Text>
            </View>
          </View>

          <View style={styles.vitalsGrid}>
            <View style={styles.vitalBox}>
              <Text style={styles.vitalLabel}>BP (SYSTOLIC)</Text>
              <Text style={styles.vitalVal}>118/76</Text>
            </View>
            <View style={styles.vitalBox}>
              <Text style={styles.vitalLabel}>HEART RATE</Text>
              <Text style={styles.vitalVal}>72 BPM</Text>
            </View>
          </View>

          <View style={styles.zkpStatusBox}>
            <Text style={styles.zkpText}>🛡️ ZKP Verified: Clinical insights revealed without PII leak.</Text>
          </View>

          <View style={styles.actionRow}>
            <TouchableOpacity style={styles.acceptBtn}>
              <Text style={styles.btnText}>⚡ Accept Proof</Text>
            </TouchableOpacity>
            <TouchableOpacity style={styles.rejectBtn}>
              <Text style={styles.btnTextOutline}>Request Re-verify</Text>
            </TouchableOpacity>
          </View>
        </View>

        {/* Verification Queue Item 2 */}
        <View style={styles.card}>
          <View style={styles.patientRow}>
            <View>
              <Text style={styles.patientName}>Marcus Thorne</Text>
              <Text style={styles.proofHash}>Proof ID: 0x4c1...b92</Text>
            </View>
            <View style={[styles.statusBadge, { backgroundColor: '#3A2E1C' }]}>
              <Text style={[styles.statusText, { color: '#FF9500' }]}>pending</Text>
            </View>
          </View>

          <View style={styles.vitalsGrid}>
            <View style={styles.vitalBox}>
              <Text style={styles.vitalLabel}>BP (SYSTOLIC)</Text>
              <Text style={styles.vitalVal}>142/92</Text>
            </View>
            <View style={styles.vitalBox}>
              <Text style={styles.vitalLabel}>HEART RATE</Text>
              <Text style={styles.vitalVal}>88 BPM</Text>
            </View>
          </View>

          <View style={styles.zkpStatusBox}>
            <Text style={styles.zkpText}>🛡️ ZKP Pending: Awaiting cryptographic node validation.</Text>
          </View>

          <View style={styles.actionRow}>
            <TouchableOpacity style={styles.acceptBtn}>
              <Text style={styles.btnText}>⚡ Accept Proof</Text>
            </TouchableOpacity>
            <TouchableOpacity style={styles.rejectBtn}>
              <Text style={styles.btnTextOutline}>Request Re-verify</Text>
            </TouchableOpacity>
          </View>
        </View>

      </ScrollView>
    </View>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#0D0D0F', paddingTop: 50 },
  header: { flexDirection: 'row', justifyContent: 'space-between', alignItems: 'center', paddingHorizontal: 20, paddingBottom: 15, borderBottomWidth: 1, borderBottomColor: '#1F1F23' },
  appTitle: { fontSize: 18, fontWeight: '900', color: '#FFFFFF', letterSpacing: 1 },
  appSubtitle: { fontSize: 11, color: '#34C759', marginTop: 2 },
  logoutBtn: { backgroundColor: '#26262B', paddingVertical: 6, paddingHorizontal: 12, borderRadius: 8 },
  logoutText: { color: '#FF453A', fontSize: 12, fontWeight: '700' },
  scrollContent: { padding: 20 },
  metricsRow: { flexDirection: 'row', justifyContent: 'space-between', marginBottom: 16 },
  metricCard: { flex: 1, backgroundColor: '#16161A', padding: 12, borderRadius: 12, marginHorizontal: 4, borderWidth: 1, borderColor: '#26262B' },
  metricLabel: { fontSize: 10, color: '#8E8E93', fontWeight: '700' },
  metricValue: { fontSize: 16, color: '#FFFFFF', fontWeight: '800', marginTop: 4 },
  searchContainer: { marginBottom: 16 },
  searchInput: { backgroundColor: '#16161A', borderWidth: 1, borderColor: '#26262B', borderRadius: 10, padding: 12, color: '#FFF', fontSize: 13 },
  card: { backgroundColor: '#16161A', borderRadius: 16, padding: 16, marginBottom: 16, borderWidth: 1, borderColor: '#26262B' },
  cardHeader: { flexDirection: 'row', justifyContent: 'space-between', alignItems: 'center', marginBottom: 12 },
  cardTitle: { fontSize: 14, fontWeight: '700', color: '#FFFFFF' },
  badgeText: { fontSize: 10, color: '#FF453A', fontWeight: '700' },
  chartMockContainer: { height: 100, backgroundColor: '#111115', borderRadius: 8, justifyContent: 'center', padding: 10 },
  chartLineMock: { height: 40, borderBottomWidth: 2, borderBottomColor: '#FF453A', justifyContent: 'flex-end', alignItems: 'center' },
  chartTooltip: { fontSize: 10, color: '#FF453A', fontWeight: '700', backgroundColor: '#26262B', paddingHorizontal: 6, paddingVertical: 2, borderRadius: 4 },
  chartAxis: { flexDirection: 'row', justifyContent: 'space-between', marginTop: 8 },
  axisText: { fontSize: 9, color: '#666' },
  patientRow: { flexDirection: 'row', justifyContent: 'space-between', alignItems: 'center', marginBottom: 12 },
  patientName: { fontSize: 16, fontWeight: '700', color: '#FFFFFF' },
  proofHash: { fontSize: 11, color: '#8E8E93', marginTop: 2 },
  statusBadge: { paddingHorizontal: 8, paddingVertical: 4, borderRadius: 6 },
  statusText: { fontSize: 11, fontWeight: '800', textTransform: 'uppercase' },
  vitalsGrid: { flexDirection: 'row', justifyContent: 'space-between', marginBottom: 12 },
  vitalBox: { flex: 1, backgroundColor: '#111115', padding: 10, borderRadius: 8, marginRight: 6 },
  vitalLabel: { fontSize: 9, color: '#8E8E93', fontWeight: '700' },
  vitalVal: { fontSize: 18, fontWeight: '800', color: '#FFFFFF', marginTop: 4 },
  zkpStatusBox: { backgroundColor: '#111115', padding: 10, borderRadius: 8, marginBottom: 12, borderWidth: 1, borderColor: '#26262B' },
  zkpText: { fontSize: 11, color: '#A0A0AB' },
  actionRow: { flexDirection: 'row', justifyContent: 'space-between' },
  acceptBtn: { flex: 1, backgroundColor: '#FF3B30', padding: 12, borderRadius: 8, alignItems: 'center', marginRight: 6 },
  rejectBtn: { flex: 1, backgroundColor: '#26262B', padding: 12, borderRadius: 8, alignItems: 'center', marginLeft: 6 },
  btnText: { color: '#FFFFFF', fontWeight: '700', fontSize: 12 },
  btnTextOutline: { color: '#8E8E93', fontWeight: '700', fontSize: 12 }