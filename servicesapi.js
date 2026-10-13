// services/api.js
const API_BASE_URL = 'http://localhost:8000/api/v1'; // เปลี่ยนเป็น IP คอมพิวเตอร์ของคุณเมื่อรันบนมือถือจริง

export const submitMedicalRecord = async (payload) => {
  try {
    const response = await fetch(`${API_BASE_URL}/medical/record`, {
      method: 'POST',
      headers: {
        'Content-Type': 'application/json',
        'Accept': 'application/json',
      },
      body: JSON.stringify(payload),
    });

    if (!response.ok) {
      throw new Error(`HTTP error! status: ${response.status}`);
    }

    const data = await response.json();
    return { success: true, data };
  } catch (error) {
    console.error('API Error (submitMedicalRecord):', error);
    return { success: false, error: error.message };
  }
};

export const fetchBlockchainLedger = async () => {
  try {
    const response = await fetch(`${API_BASE_URL}/blockchain/ledger`);
    const data = await response.json();
    return { success: true, data };
  } catch (error) {
    console.error('API Error (fetchBlockchainLedger):', error);
    return { success: false, error: error.message };
  }
};// services/api.js
const API_BASE_URL = 'http://localhost:8000/api/v1'; // เปลี่ยนเป็น IP คอมพิวเตอร์ของคุณเมื่อรันบนมือถือจริง

export const submitMedicalRecord = async (payload) => {
  try {
    const response = await fetch(`${API_BASE_URL}/medical/record`, {
      method: 'POST',
      headers: {
        'Content-Type': 'application/json',
        'Accept': 'application/json',
      },
      body: JSON.stringify(payload),
    });

    if (!response.ok) {
      throw new Error(`HTTP error! status: ${response.status}`);
    }

    const data = await response.json();
    return { success: true, data };
  } catch (error) {
    console.error('API Error (submitMedicalRecord):', error);
    return { success: false, error: error.message };
  }
};

export const fetchBlockchainLedger = async () => {
  try {
    const response = await fetch(`${API_BASE_URL}/blockchain/ledger`);
    const data = await response.json();
    return { success: true, data };
  } catch (error) {
    console.error('API Error (fetchBlockchainLedger):', error);
    return { success: false, error: error.message };
  }
};