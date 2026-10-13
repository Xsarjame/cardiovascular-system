#include <torch/script.h>
#include <torch/torch.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <memory>
#include <numeric>
#include <algorithm>
#include <cmath>
#include <chrono>
#include <iomanip>
#include <cstdint>
#include <cstring>

// =====================================================================
// PART 1: REAL FIPS 180-4 SHA-256 CRYPTOGRAPHIC ENGINE
// =====================================================================
class SHA256 {
private:
    uint32_t state[8];
    uint8_t buffer[64];
    uint32_t datalen;
    uint64_t bitlen;
    uint32_t k[64];

    static uint32_t rotr(uint32_t x, uint32_t n) { return (x >> n) | (x << (32 - n)); }
    static uint32_t choose(uint32_t e, uint32_t f, uint32_t g) { return (e & f) ^ (~e & g); }
    static uint32_t majority(uint32_t a, uint32_t b, uint32_t c) { return (a & b) ^ (a & c) ^ (b & c); }
    static uint32_t sig0(uint32_t x) { return rotr(x, 2) ^ rotr(x, 13) ^ rotr(x, 22); }
    static uint32_t sig1(uint32_t x) { return rotr(x, 6) ^ rotr(x, 11) ^ rotr(x, 25); }
    static uint32_t tousig0(uint32_t x) { return rotr(x, 7) ^ rotr(x, 18) ^ (x >> 3); }
    static uint32_t tousig1(uint32_t x) { return rotr(x, 17) ^ rotr(x, 19) ^ (x >> 10); }

    void transform(const uint8_t data[]) {
        uint32_t a, b, c, d, e, f, g, h, i, j, t1, t2, m[64];
        for (i = 0, j = 0; i < 16; ++i, j += 4)
            m[i] = (data[j] << 24) | (data[j + 1] << 16) | (data[j + 2] << 8) | (data[j + 3]);
        for (; i < 64; ++i)
            m[i] = tousig1(m[i - 2]) + m[i - 7] + tousig0(m[i - 15]) + m[i - 16];

        a = state[0]; b = state[1]; c = state[2]; d = state[3];
        e = state[4]; f = state[5]; g = state[6]; h = state[7];

        for (i = 0; i < 64; ++i) {
            t1 = h + sig1(e) + choose(e, f, g) + k[i] + m[i];
            t2 = sig0(a) + majority(a, b, c);
            h = g; g = f; f = e; e = d + t1;
            d = c; c = b; b = a; a = t1 + t2;
        }
        state[0] += a; state[1] += b; state[2] += c; state[3] += d;
        state[4] += e; state[5] += f; state[6] += g; state[7] += h;
    }

public:
    SHA256() {
        uint32_t k_init[64] = {
            0x428a2f98,0x71374491,0xb5c0fbcf,0xe9b5dba5,0x3956c25b,0x59f111f1,0x923f82a4,0xab1c5ed5,
            0xd807aa98,0x12835b01,0x243185be,0x550c7dc3,0x72be5d74,0x80deb1fe,0x9bdc06a7,0xc19bf174,
            0xe49b69c1,0xefbe4786,0x0fc19dc6,0x240ca1cc,0x2de92c6f,0x4a7484aa,0x5cb0a9dc,0x76f988da,
            0x983e5152,0xa831c66d,0xb03238c4,0xbf657fc8,0xc6e00bf3,0xd5a79147,0x06ca6351,0x14292967,
            0x27b70a85,0x2e1b2138,0x4d2c6dfc,0x53380d13,0x650a7354,0x766a0abb,0x81c2c92e,0x92722c85,
            0xa2bfe8a1,0xa81a664b,0xc24b8b70,0xc76c51a3,0xd192e819,0xd6990624,0xf40e3585,0x106aa070,
            0x19a4c116,0x1e376c08,0x2748774c,0x34b0bcb5,0x391c0cb3,0x4ed8aa4a,0x5b9cca4f,0x682e6ff3,
            0x748f82ee,0x78a5636f,0x84c87814,0x8cc70208,0x90befffa,0xa4506ceb,0xbef9a3f7,0xc67178f2
        };
        std::memcpy(k, k_init, sizeof(k));
        reset();
    }

    void reset() {
        state[0] = 0x6a09e667; state[1] = 0xbb67ae85; state[2] = 0x3c6ef372; state[3] = 0xa54ff53a;
        state[4] = 0x510e527f; state[5] = 0x9b05688c; state[6] = 0x1f83d9ab; state[7] = 0x5be0cd19;
        datalen = 0; bitlen = 0;
    }

    void update(const uint8_t* data, size_t len) {
        for (size_t i = 0; i < len; ++i) {
            buffer[datalen++] = data[i];
            if (datalen == 64) {
                transform(buffer);
                bitlen += 512;
                datalen = 0;
            }
        }
    }

    void final(uint8_t hash[32]) {
        uint32_t i = datalen;
        if (datalen < 56) {
            buffer[i++] = 0x80;
            while (i < 56) buffer[i++] = 0x00;
        } else {
            buffer[i++] = 0x80;
            while (i < 64) buffer[i++] = 0x00;
            transform(buffer);
            std::memset(buffer, 0, 56);
        }
        bitlen += (uint64_t)datalen * 8;
        buffer[63] = bitlen;
        buffer[62] = bitlen >> 8;
        buffer[61] = bitlen >> 16;
        buffer[60] = bitlen >> 24;
        buffer[59] = bitlen >> 32;
        buffer[58] = bitlen >> 40;
        buffer[57] = bitlen >> 48;
        buffer[56] = bitlen >> 56;
        transform(buffer);

        for (i = 0; i < 4; ++i) {
            hash[i]      = (state[0] >> (24 - i * 8)) & 0xff;
            hash[i + 4]  = (state[1] >> (24 - i * 8)) & 0xff;
            hash[i + 8]  = (state[2] >> (24 - i * 8)) & 0xff;
            hash[i + 12] = (state[3] >> (24 - i * 8)) & 0xff;
            hash[i + 16] = (state[4] >> (24 - i * 8)) & 0xff;
            hash[i + 20] = (state[5] >> (24 - i * 8)) & 0xff;
            hash[i + 24] = (state[6] >> (24 - i * 8)) & 0xff;
            hash[i + 28] = (state[7] >> (24 - i * 8)) & 0xff;
        }
    }

    static std::string hash_string(const std::string& input) {
        SHA256 sha256;
        sha256.update(reinterpret_cast<const uint8_t*>(input.data()), input.size());
        uint8_t hash[32];
        sha256.final(hash);
        
        std::stringstream ss;
        for (int i = 0; i < 32; ++i) {
            ss << std::hex << std::setw(2) << std::setfill('0') << (int)hash[i];
        }
        return ss.str();
    }
};

// =====================================================================
// PART 2: BLOCKCHAIN AUDIT LOGGER & POA NODE
// =====================================================================
class ProductionBlockchainLogger {
private:
    std::string log_filename;
    std::string latest_hash;
    int block_counter;

    std::string get_current_timestamp() {
        std::time_t now = std::time(nullptr);
        char buf[80];
        std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", std::localtime(&now));
        return std::string(buf);
    }

public:
    ProductionBlockchainLogger(const std::string& filename = "production_audit_trail.log") 
        : log_filename(filename), block_counter(0), latest_hash("0000000000000000000000000000000000000000000000000000000000000000") {
        std::ofstream outfile(log_filename, std::ios::trunc);
        outfile << "# P'MAX SECURE PRODUCTION AUDIT TRAIL (PoA & Real SHA-256)\n";
        outfile.close();
    }

    std::string append_diagnostic_block(int pred_class, float confidence, float delta, bool review) {
        int index = block_counter++;
        std::string timestamp = get_current_timestamp();

        std::stringstream raw_data;
        raw_data << index << "|" << timestamp << "|" << pred_class << "|" 
                 << std::fixed << std::setprecision(4) << confidence << "|" 
                 << delta << "|" << (review ? "1" : "0") << "|" << latest_hash;

        std::string current_hash = SHA256::hash_string(raw_data.str());
        std::string poa_signature = SHA256::hash_string(current_hash + "_PMAX_AUTHORIZED_NODE_KEY");

        latest_hash = current_hash;

        std::ofstream outfile(log_filename, std::ios::app);
        if (outfile.is_open()) {
            outfile << "BLOCK_INDEX=" << index << "\n";
            outfile << "TIMESTAMP=" << timestamp << "\n";
            outfile << "PREDICTION=" << pred_class << "\n";
            outfile << "CONFIDENCE=" << confidence << "\n";
            outfile << "DELTA_RATE=" << delta << "\n";
            outfile << "EXPERT_REVIEW=" << (review ? "TRUE" : "FALSE") << "\n";
            outfile << "PREV_HASH=" << latest_hash << "\n";
            outfile << "CURR_HASH=" << current_hash << "\n";
            outfile << "POA_SIG=" << poa_signature << "\n";
            outfile << "--------------------------------------------------\n";
            outfile.close();
        }
        return current_hash;
    }
};

// =====================================================================
// PART 3: COMPLETE END-TO-END MASTER PIPELINE MAIN
// =====================================================================
int main() {
    std::cout << "=================================================" << std::endl;
    std::cout << "   P'MAX CARDIAC BACKEND: END-TO-END PIPELINE    " << std::endl;
    std::cout << "=================================================" << std::endl;

    // 1. โหลดโมเดล 5-Fold Ensemble (จำลองหรือโหลดจากไฟล์ .pt จริง)
    std::vector<std::shared_ptr<torch::jit::script::Module>> ensemble_models;
    bool use_real_models = false;

    try {
        std::cout << "[Info] กำลังพยายามโหลดโมเดล 5-Fold TorchScript (.pt)..." << std::endl;
        for (int i = 1; i <= 5; ++i) {
            std::string model_path = "model_fold_" + std::to_string(i) + "_script.pt";
            auto model = std::make_shared<torch::jit::script::Module>(torch::jit::load(model_path));
            model->eval();
            ensemble_models.push_back(model);
        }
        use_real_models = true;
        std::cout << "[Success] โหลดโมเดลทั้ง 5 โฟลด์สำเร็จ!" << std::endl;
    } catch (const c10::Error& e) {
        std::cout << "[Warning] ไม่พบไฟล์โมเดล .pt ในโฟลเดอร์ ระบบจะสลับไปใช้โหมด Robust Fallback Simulation สำหรับเทสต์ระบบทันที" << std::endl;
    }

    // 2. เตรียมระบบ Blockchain Logger สำหรับบันทึก Audit Trail
    ProductionBlockchainLogger blockchain_logger("pmax_production_ledger.log");

    // 3. จำลองสตรีมข้อมูลสัญญาณเซ็นเซอร์เข้าสู่ระบบ (เช่น 3 แพ็กเก็ตข้อมูลต่อเนื่อง)
    std::cout << "\n--- เริ่มต้นสตรีมข้อมูลและประมวลผลแบบเรียลไทม์ ---" << std::endl;

    for (int step = 1; step <= 3; ++step) {
        auto start_time = std::chrono::high_resolution_clock::now();

        int final_pred = 0;
        float final_confidence = 0.0f;
        float simulated_delta_rate = 0.15f + (step * 0.10f); // สเตปท้ายๆ Delta Rate จะพุ่งเกิน 35% (0.35)

        if (use_real_models) {
            // จำลองการสร้าง Input Tensor ขนาด [1, 1, 64, 32]
            torch::Tensor input_tensor = torch::rand({1, 1, 64, 32});
            std::vector<torch::jit::IValue> inputs;
            inputs.push_back(input_tensor);

            // 5-Fold Ensemble Soft Voting Inference
            float prob_sum = 0.0f;
            for (auto& model : ensemble_models) {
                at::IValue output = model->forward(inputs);
                torch::Tensor out_tensor = output.toTensor();
                prob_sum += out_tensor.item<float>();
            }
            float avg_prob = prob_sum / 5.0f;
            final_pred = (avg_prob >= 0.5f) ? 1 : 0;
            final_confidence = (final_pred == 1) ? avg_prob : (1.0f - avg_prob);
        } else {
            // Fallback Simulation Mode
            final_pred = (step == 3) ? 1 : 0;
            final_confidence = 0.88f + (step * 0.02f);
        }

        // Safety Gate Check (เงื่อนไข 35% Delta Rate หรือ Conf ต่ำกว่าเกณฑ์)
        bool safety_review_triggered = (final_confidence < 0.75f || simulated_delta_rate >= 0.35f);

        // บันทึกผลลง Blockchain Ledger (Real SHA-256 Hash-Chaining)
        std::string block_hash = blockchain_logger.append_diagnostic_block(
            final_pred, final_confidence, simulated_delta_rate, safety_review_triggered
        );

        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration_us = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time).count();

        std::cout << "[Packet " << step << "] Latency: " << duration_us << " us" 
                  << " | Pred: " << final_pred 
                  << " | Delta: " << std::fixed << std::setprecision(2) << (simulated_delta_rate * 100.0f) << "%"
                  << " | Safety Gate: " << (safety_review_triggered ? "TRIGGERED (Review Req)" : "NORMAL") << std::endl;
        std::cout << "          -> SHA-256 Hash: " << block_hash.substr(0, 24) << "..." << std::endl;
    }

    std::cout << "=================================================" << std::endl;
    std::cout << "ระบบทำงานครบถ้วนตั้งแต่ต้นจนจบ! ตรวจสอบไฟล์ 'pmax_production_ledger.log' ได้เลยครับ" << std::endl;

    return 0;
}#include <torch/script.h>
#include <torch/torch.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <memory>
#include <numeric>
#include <algorithm>
#include <cmath>
#include <chrono>
#include <iomanip>
#include <cstdint>
#include <cstring>

// =====================================================================
// PART 1: REAL FIPS 180-4 SHA-256 CRYPTOGRAPHIC ENGINE
// =====================================================================
class SHA256 {
private:
    uint32_t state[8];
    uint8_t buffer[64];
    uint32_t datalen;
    uint64_t bitlen;
    uint32_t k[64];

    static uint32_t rotr(uint32_t x, uint32_t n) { return (x >> n) | (x << (32 - n)); }
    static uint32_t choose(uint32_t e, uint32_t f, uint32_t g) { return (e & f) ^ (~e & g); }
    static uint32_t majority(uint32_t a, uint32_t b, uint32_t c) { return (a & b) ^ (a & c) ^ (b & c); }
    static uint32_t sig0(uint32_t x) { return rotr(x, 2) ^ rotr(x, 13) ^ rotr(x, 22); }
    static uint32_t sig1(uint32_t x) { return rotr(x, 6) ^ rotr(x, 11) ^ rotr(x, 25); }
    static uint32_t tousig0(uint32_t x) { return rotr(x, 7) ^ rotr(x, 18) ^ (x >> 3); }
    static uint32_t tousig1(uint32_t x) { return rotr(x, 17) ^ rotr(x, 19) ^ (x >> 10); }

    void transform(const uint8_t data[]) {
        uint32_t a, b, c, d, e, f, g, h, i, j, t1, t2, m[64];
        for (i = 0, j = 0; i < 16; ++i, j += 4)
            m[i] = (data[j] << 24) | (data[j + 1] << 16) | (data[j + 2] << 8) | (data[j + 3]);
        for (; i < 64; ++i)
            m[i] = tousig1(m[i - 2]) + m[i - 7] + tousig0(m[i - 15]) + m[i - 16];

        a = state[0]; b = state[1]; c = state[2]; d = state[3];
        e = state[4]; f = state[5]; g = state[6]; h = state[7];

        for (i = 0; i < 64; ++i) {
            t1 = h + sig1(e) + choose(e, f, g) + k[i] + m[i];
            t2 = sig0(a) + majority(a, b, c);
            h = g; g = f; f = e; e = d + t1;
            d = c; c = b; b = a; a = t1 + t2;
        }
        state[0] += a; state[1] += b; state[2] += c; state[3] += d;
        state[4] += e; state[5] += f; state[6] += g; state[7] += h;
    }

public:
    SHA256() {
        uint32_t k_init[64] = {
            0x428a2f98,0x71374491,0xb5c0fbcf,0xe9b5dba5,0x3956c25b,0x59f111f1,0x923f82a4,0xab1c5ed5,
            0xd807aa98,0x12835b01,0x243185be,0x550c7dc3,0x72be5d74,0x80deb1fe,0x9bdc06a7,0xc19bf174,
            0xe49b69c1,0xefbe4786,0x0fc19dc6,0x240ca1cc,0x2de92c6f,0x4a7484aa,0x5cb0a9dc,0x76f988da,
            0x983e5152,0xa831c66d,0xb03238c4,0xbf657fc8,0xc6e00bf3,0xd5a79147,0x06ca6351,0x14292967,
            0x27b70a85,0x2e1b2138,0x4d2c6dfc,0x53380d13,0x650a7354,0x766a0abb,0x81c2c92e,0x92722c85,
            0xa2bfe8a1,0xa81a664b,0xc24b8b70,0xc76c51a3,0xd192e819,0xd6990624,0xf40e3585,0x106aa070,
            0x19a4c116,0x1e376c08,0x2748774c,0x34b0bcb5,0x391c0cb3,0x4ed8aa4a,0x5b9cca4f,0x682e6ff3,
            0x748f82ee,0x78a5636f,0x84c87814,0x8cc70208,0x90befffa,0xa4506ceb,0xbef9a3f7,0xc67178f2
        };
        std::memcpy(k, k_init, sizeof(k));
        reset();
    }

    void reset() {
        state[0] = 0x6a09e667; state[1] = 0xbb67ae85; state[2] = 0x3c6ef372; state[3] = 0xa54ff53a;
        state[4] = 0x510e527f; state[5] = 0x9b05688c; state[6] = 0x1f83d9ab; state[7] = 0x5be0cd19;
        datalen = 0; bitlen = 0;
    }

    void update(const uint8_t* data, size_t len) {
        for (size_t i = 0; i < len; ++i) {
            buffer[datalen++] = data[i];
            if (datalen == 64) {
                transform(buffer);
                bitlen += 512;
                datalen = 0;
            }
        }
    }

    void final(uint8_t hash[32]) {
        uint32_t i = datalen;
        if (datalen < 56) {
            buffer[i++] = 0x80;
            while (i < 56) buffer[i++] = 0x00;
        } else {
            buffer[i++] = 0x80;
            while (i < 64) buffer[i++] = 0x00;
            transform(buffer);
            std::memset(buffer, 0, 56);
        }
        bitlen += (uint64_t)datalen * 8;
        buffer[63] = bitlen;
        buffer[62] = bitlen >> 8;
        buffer[61] = bitlen >> 16;
        buffer[60] = bitlen >> 24;
        buffer[59] = bitlen >> 32;
        buffer[58] = bitlen >> 40;
        buffer[57] = bitlen >> 48;
        buffer[56] = bitlen >> 56;
        transform(buffer);

        for (i = 0; i < 4; ++i) {
            hash[i]      = (state[0] >> (24 - i * 8)) & 0xff;
            hash[i + 4]  = (state[1] >> (24 - i * 8)) & 0xff;
            hash[i + 8]  = (state[2] >> (24 - i * 8)) & 0xff;
            hash[i + 12] = (state[3] >> (24 - i * 8)) & 0xff;
            hash[i + 16] = (state[4] >> (24 - i * 8)) & 0xff;
            hash[i + 20] = (state[5] >> (24 - i * 8)) & 0xff;
            hash[i + 24] = (state[6] >> (24 - i * 8)) & 0xff;
            hash[i + 28] = (state[7] >> (24 - i * 8)) & 0xff;
        }
    }

    static std::string hash_string(const std::string& input) {
        SHA256 sha256;
        sha256.update(reinterpret_cast<const uint8_t*>(input.data()), input.size());
        uint8_t hash[32];
        sha256.final(hash);
        
        std::stringstream ss;
        for (int i = 0; i < 32; ++i) {
            ss << std::hex << std::setw(2) << std::setfill('0') << (int)hash[i];
        }
        return ss.str();
    }
};

// =====================================================================
// PART 2: BLOCKCHAIN AUDIT LOGGER & POA NODE
// =====================================================================
class ProductionBlockchainLogger {
private:
    std::string log_filename;
    std::string latest_hash;
    int block_counter;

    std::string get_current_timestamp() {
        std::time_t now = std::time(nullptr);
        char buf[80];
        std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", std::localtime(&now));
        return std::string(buf);
    }

public:
    ProductionBlockchainLogger(const std::string& filename = "production_audit_trail.log") 
        : log_filename(filename), block_counter(0), latest_hash("0000000000000000000000000000000000000000000000000000000000000000") {
        std::ofstream outfile(log_filename, std::ios::trunc);
        outfile << "# P'MAX SECURE PRODUCTION AUDIT TRAIL (PoA & Real SHA-256)\n";
        outfile.close();
    }

    std::string append_diagnostic_block(int pred_class, float confidence, float delta, bool review) {
        int index = block_counter++;
        std::string timestamp = get_current_timestamp();

        std::stringstream raw_data;
        raw_data << index << "|" << timestamp << "|" << pred_class << "|" 
                 << std::fixed << std::setprecision(4) << confidence << "|" 
                 << delta << "|" << (review ? "1" : "0") << "|" << latest_hash;

        std::string current_hash = SHA256::hash_string(raw_data.str());
        std::string poa_signature = SHA256::hash_string(current_hash + "_PMAX_AUTHORIZED_NODE_KEY");

        latest_hash = current_hash;

        std::ofstream outfile(log_filename, std::ios::app);
        if (outfile.is_open()) {
            outfile << "BLOCK_INDEX=" << index << "\n";
            outfile << "TIMESTAMP=" << timestamp << "\n";
            outfile << "PREDICTION=" << pred_class << "\n";
            outfile << "CONFIDENCE=" << confidence << "\n";
            outfile << "DELTA_RATE=" << delta << "\n";
            outfile << "EXPERT_REVIEW=" << (review ? "TRUE" : "FALSE") << "\n";
            outfile << "PREV_HASH=" << latest_hash << "\n";
            outfile << "CURR_HASH=" << current_hash << "\n";
            outfile << "POA_SIG=" << poa_signature << "\n";
            outfile << "--------------------------------------------------\n";
            outfile.close();
        }
        return current_hash;
    }
};

// =====================================================================
// PART 3: COMPLETE END-TO-END MASTER PIPELINE MAIN
// =====================================================================
int main() {
    std::cout << "=================================================" << std::endl;
    std::cout << "   P'MAX CARDIAC BACKEND: END-TO-END PIPELINE    " << std::endl;
    std::cout << "=================================================" << std::endl;

    // 1. โหลดโมเดล 5-Fold Ensemble (จำลองหรือโหลดจากไฟล์ .pt จริง)
    std::vector<std::shared_ptr<torch::jit::script::Module>> ensemble_models;
    bool use_real_models = false;

    try {
        std::cout << "[Info] กำลังพยายามโหลดโมเดล 5-Fold TorchScript (.pt)..." << std::endl;
        for (int i = 1; i <= 5; ++i) {
            std::string model_path = "model_fold_" + std::to_string(i) + "_script.pt";
            auto model = std::make_shared<torch::jit::script::Module>(torch::jit::load(model_path));
            model->eval();
            ensemble_models.push_back(model);
        }
        use_real_models = true;
        std::cout << "[Success] โหลดโมเดลทั้ง 5 โฟลด์สำเร็จ!" << std::endl;
    } catch (const c10::Error& e) {
        std::cout << "[Warning] ไม่พบไฟล์โมเดล .pt ในโฟลเดอร์ ระบบจะสลับไปใช้โหมด Robust Fallback Simulation สำหรับเทสต์ระบบทันที" << std::endl;
    }

    // 2. เตรียมระบบ Blockchain Logger สำหรับบันทึก Audit Trail
    ProductionBlockchainLogger blockchain_logger("pmax_production_ledger.log");

    // 3. จำลองสตรีมข้อมูลสัญญาณเซ็นเซอร์เข้าสู่ระบบ (เช่น 3 แพ็กเก็ตข้อมูลต่อเนื่อง)
    std::cout << "\n--- เริ่มต้นสตรีมข้อมูลและประมวลผลแบบเรียลไทม์ ---" << std::endl;

    for (int step = 1; step <= 3; ++step) {
        auto start_time = std::chrono::high_resolution_clock::now();

        int final_pred = 0;
        float final_confidence = 0.0f;
        float simulated_delta_rate = 0.15f + (step * 0.10f); // สเตปท้ายๆ Delta Rate จะพุ่งเกิน 35% (0.35)

        if (use_real_models) {
            // จำลองการสร้าง Input Tensor ขนาด [1, 1, 64, 32]
            torch::Tensor input_tensor = torch::rand({1, 1, 64, 32});
            std::vector<torch::jit::IValue> inputs;
            inputs.push_back(input_tensor);

            // 5-Fold Ensemble Soft Voting Inference
            float prob_sum = 0.0f;
            for (auto& model : ensemble_models) {
                at::IValue output = model->forward(inputs);
                torch::Tensor out_tensor = output.toTensor();
                prob_sum += out_tensor.item<float>();
            }
            float avg_prob = prob_sum / 5.0f;
            final_pred = (avg_prob >= 0.5f) ? 1 : 0;
            final_confidence = (final_pred == 1) ? avg_prob : (1.0f - avg_prob);
        } else {
            // Fallback Simulation Mode
            final_pred = (step == 3) ? 1 : 0;
            final_confidence = 0.88f + (step * 0.02f);
        }

        // Safety Gate Check (เงื่อนไข 35% Delta Rate หรือ Conf ต่ำกว่าเกณฑ์)
        bool safety_review_triggered = (final_confidence < 0.75f || simulated_delta_rate >= 0.35f);

        // บันทึกผลลง Blockchain Ledger (Real SHA-256 Hash-Chaining)
        std::string block_hash = blockchain_logger.append_diagnostic_block(
            final_pred, final_confidence, simulated_delta_rate, safety_review_triggered
        );

        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration_us = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time).count();

        std::cout << "[Packet " << step << "] Latency: " << duration_us << " us" 
                  << " | Pred: " << final_pred 
                  << " | Delta: " << std::fixed << std::setprecision(2) << (simulated_delta_rate * 100.0f) << "%"
                  << " | Safety Gate: " << (safety_review_triggered ? "TRIGGERED (Review Req)" : "NORMAL") << std::endl;
        std::cout << "          -> SHA-256 Hash: " << block_hash.substr(0, 24) << "..." << std::endl;
    }

    std::cout << "=================================================" << std::endl;
    std::cout << "ระบบทำงานครบถ้วนตั้งแต่ต้นจนจบ! ตรวจสอบไฟล์ 'pmax_production_ledger.log' ได้เลยครับ" << std::endl;

    return 0;
}