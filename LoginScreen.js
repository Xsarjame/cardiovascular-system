// screens/LoginScreen.js
import React, { useState } from 'react';
import { StyleSheet, Text, View, TextInput, TouchableOpacity, ActivityIndicator, Alert } from 'react-native';
import { loginUser } from '../services/authService';

export default function LoginScreen({ onLoginSuccess }) {
  const [username, setUsername] = useState('patient01');
  const [password, setPassword] = useState('secure123');
  const [loading, setLoading] = useState(false);

  const handleLogin = async () => {
    setLoading(true);
    const result = await loginUser(username, password);
    setLoading(false);

    if (result.success) {
      onLoginSuccess(result.session);
    } else {
      Alert.alert("เข้าสู่ระบบไม่สำเร็จ", result.error);
    }
  };

  return (
    <View style={styles.container}>
      <View style={styles.card}>
        <Text style={styles.title}>VITALPULSE ZKP</Text>
        <Text style={styles.subtitle}>Secure Medical Gateway Access</Text>

        <Text style={styles.label}>Patient ID / Username</Text>
        <TextInput 
          style={styles.input} 
          value={username} 
          onChangeText={setUsername} 
          placeholder="Enter username" 
        />

        <Text style={styles.label}>Secure Password</Text>
        <TextInput 
          style={styles.input} 
          value={password} 
          onChangeText={setPassword} 
          secureTextEntry 
          placeholder="Enter password" 
        />

        <TouchableOpacity style={styles.button} onPress={handleLogin} disabled={loading}>
          {loading ? <ActivityIndicator color="#FFF" /> : <Text style={styles.buttonText}>Secure Login</Text>}
        </TouchableOpacity>
      </View>
    </View>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#FAFAFC', justifyContent: 'center', padding: 20 },
  card: { backgroundColor: '#FFFFFF', padding: 24, borderRadius: 16, borderWidth: 1, borderColor: '#E5E5EA' },
  title: { fontSize: 22, fontWeight: '800', color: '#1A1A1A', textAlign: 'center', letterSpacing: 1.5 },
  subtitle: { fontSize: 12, color: '#666', textAlign: 'center', marginBottom: 24, marginTop: 4 },
  label: { fontSize: 11, fontWeight: '700', color: '#8E8E93', marginBottom: 6, letterSpacing: 0.5 },
  input: { backgroundColor: '#F8F9FA', borderWidth: 1, borderColor: '#E5E5EA', borderRadius: 10, padding: 12, fontSize: 14, color: '#1A1A1A', marginBottom: 16 },
  button: { backgroundColor: '#8B0000', borderRadius: 10, padding: 14, alignItems: 'center', marginTop: 10 },
  buttonText: { color: '#FFFFFF', fontWeight: '700', fontSize: 14 }
});// screens/LoginScreen.js
import React, { useState } from 'react';
import { StyleSheet, Text, View, TextInput, TouchableOpacity, ActivityIndicator, Alert } from 'react-native';
import { loginUser } from '../services/authService';

export default function LoginScreen({ onLoginSuccess }) {
  const [username, setUsername] = useState('patient01');
  const [password, setPassword] = useState('secure123');
  const [loading, setLoading] = useState(false);

  const handleLogin = async () => {
    setLoading(true);
    const result = await loginUser(username, password);
    setLoading(false);

    if (result.success) {
      onLoginSuccess(result.session);
    } else {
      Alert.alert("เข้าสู่ระบบไม่สำเร็จ", result.error);
    }
  };

  return (
    <View style={styles.container}>
      <View style={styles.card}>
        <Text style={styles.title}>VITALPULSE ZKP</Text>
        <Text style={styles.subtitle}>Secure Medical Gateway Access</Text>

        <Text style={styles.label}>Patient ID / Username</Text>
        <TextInput 
          style={styles.input} 
          value={username} 
          onChangeText={setUsername} 
          placeholder="Enter username" 
        />

        <Text style={styles.label}>Secure Password</Text>
        <TextInput 
          style={styles.input} 
          value={password} 
          onChangeText={setPassword} 
          secureTextEntry 
          placeholder="Enter password" 
        />

        <TouchableOpacity style={styles.button} onPress={handleLogin} disabled={loading}>
          {loading ? <ActivityIndicator color="#FFF" /> : <Text style={styles.buttonText}>Secure Login</Text>}
        </TouchableOpacity>
      </View>
    </View>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#FAFAFC', justifyContent: 'center', padding: 20 },
  card: { backgroundColor: '#FFFFFF', padding: 24, borderRadius: 16, borderWidth: 1, borderColor: '#E5E5EA' },
  title: { fontSize: 22, fontWeight: '800', color: '#1A1A1A', textAlign: 'center', letterSpacing: 1.5 },
  subtitle: { fontSize: 12, color: '#666', textAlign: 'center', marginBottom: 24, marginTop: 4 },
  label: { fontSize: 11, fontWeight: '700', color: '#8E8E93', marginBottom: 6, letterSpacing: 0.5 },
  input: { backgroundColor: '#F8F9FA', borderWidth: 1, borderColor: '#E5E5EA', borderRadius: 10, padding: 12, fontSize: 14, color: '#1A1A1A', marginBottom: 16 },
  button: { backgroundColor: '#8B0000', borderRadius: 10, padding: 14, alignItems: 'center', marginTop: 10 },
  buttonText: { color: '#FFFFFF', fontWeight: '700', fontSize: 14 }
});