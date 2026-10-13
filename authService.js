// services/authService.js
import AsyncStorage from '@react-native-async-storage/async-storage';

export const loginUser = async (identifier, password) => {
  // จำลองการตรวจสอบสิทธิ์กับ Backend
  if (identifier === "patient01" && password === "secure123") {
    const userSession = {
      token: "jwt_token_sample_9921",
      patient_id: "PATIENT-TH-9921",
      name: "คุณสมชาย ใจดี"
    };
    await AsyncStorage.setItem('user_session', JSON.stringify(userSession));
    return { success: true, session: userSession };
  }
  return { success: false, error: "รหัสผ่านหรือชื่อผู้ใช้ไม่ถูกต้อง" };
};

export const logoutUser = async () => {
  await AsyncStorage.removeItem('user_session');
};