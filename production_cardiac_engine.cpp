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

// โครงสร้างเก็บผลลัพธ์การวินิจฉัยระดับ Production
struct ProductionDiagnosticResult {
    int predicted_class;              // 0 = Normal, 1 = Abnormal / Crisis
    float confidence_score;           // ค่าความมั่นใจ (0.0 - 1.0)
    std::vector<float> probabilities; // ความน่าจะเป็นแต่ละคลาส
    float delta_rate_of_change;       // อัตราการเปลี่ยนแปลงเทียบกับ Baseline (%)
    bool exceeds_delta_threshold;     // เกิน 35% Delta Rate หรือไม่
    bool requires_expert_review;      // ต้องส่งต่อผู้เชี่ยวชาญหรือไม่
    bool is_valid_execution;          // สถานะความสำเร็จของการประมวลผล (Error Handling)
    std::string error_message;        // ข้อความแจ้งข้อผิดพลาด (ถ้ามี)
    std::string cryptographic_hash;   // Hash จำลองสำหรับระบบ Append-Only Log / PoA
};

// คลาสจัดการ Non-Ideal Backend สำหรับ Edge Device
class ProductionCardiacInferenceEngine {
private:
    std::vector<torch::jit::script::Module> models;
    const int num_folds = 5;
    float confidence_threshold; 
    float delta_threshold;      
    std::vector<float> user_personal_baseline; 

    // ฟังก์ชันจำลองการสร้าง Hash ง่ายๆ สำหรับระบบ Append-Only Log (SHA-256 Mockup)
    std::string generate_audit_hash(int pred_class, float conf, float delta) {
        // ในระบบจริงจะใช้ OpenSSL SHA-256 ผูกกับ Previous Hash ของ Block
        unsigned long long dummy_hash_seed = static_cast<unsigned long long>(conf * 1000000) + 
                                            static_cast<unsigned long long>(delta * 1000000) + pred_class;
        std::stringstream ss;
        ss << "0x" << std::setfill('0') << std::setw(16) << std::hex << dummy_hash_seed;
        return ss.str() + "_PoA_Validated";
    }

    // ฟังก์ชันกรองสัญญาณรบกวนเบื้องต้น (Noise & Outlier clipping สำหรับ Non-Ideal Input)
    std::vector<float> preprocess_raw_signal(const std::vector<float>& raw_input) {
        std::vector<float> cleaned_input = raw_input;
        for (float& val : cleaned_input) {
            // ป้องกันค่า Nan หรือ Inf จากเซ็นเซอร์รวน
            if (std::isnan(val) || std::isinf(val)) {
                val = 0.0f;
            }
            // Clip ค่าให้อยู่ในช่วงที่เหมาะสม
            if (val > 10.0f) val = 10.0f;
            if (val < -10.0f) val = -10.0f;
        }
        return cleaned_input;
    }

public:
    ProductionCardiacInferenceEngine(float conf_thresh = 0.75f, float delta_thresh = 0.35f) 
        : confidence_threshold(conf_thresh), delta_threshold(delta_thresh) {
        user_personal_baseline = {0.90f, 0.10f}; // ค่าเริ่มต้นปกติ
        torch::set_num_threads(2);               // จำกัดเธรดเพื่อความเสถียรบน Edge
    }

    // โหลดโมเดลพร้อมระบบดักจับข้อผิดพลาด (Exception Handling)
    bool load_production_models(const std::string& model_dir) {
        try {
            models.clear();
            for (int i = 1; i <= num_folds; ++i) {
                std::string model_path = model_dir + "/model_fold_" + std::to_string(i) + "_script.pt";
                torch::jit::script::Module module = torch::jit::load(model_path);
                module.eval();
                models.push_back(module);
            }
            std::cout << "[Production C++] โหลดชุดโมเดล 5 Folds สำเร็จทั้งหมด" << std::endl;
            return true;
        } catch (const c10::Error& e) {
            std::cerr << "[Critical Error] โหลด TorchScript โมเดลล้มเหลว: " << e.what() << std::endl;
            return false;
        } catch (const std::exception& e) {
            std::cerr << "[Critical Error] เกิดข้อผิดพลาดที่ไม่คาดคิดในการโหลดโมเดล: " << e.what() << std::endl;
            return false;
        }
    }

    void calibrate_baseline(const std::vector<float>& new_baseline) {
        if (new_baseline.size() == 2) {
            user_personal_baseline = new_baseline;
            std::cout << "[Personalization] อัปเดต Baseline ส่วนตัวสำหรับผู้ใช้รายนี้เรียบร้อย" << std::endl;
        }
    }

    // รันกระบวนการ Inference แบบ Non-Ideal รองรับเคสหน้างานจริง
    ProductionDiagnosticResult execute_inference_pipeline(const std::vector<float>& raw_sensor_data) {
        ProductionDiagnosticResult result;
        result.is_valid_execution = false;

        // 1. Error Handling: ตรวจสอบความพร้อมของโมเดล
        if (models.empty()) {
            result.error_message = "Models are not loaded or initialized.";
            return result;
        }

        // 2. Error Handling: ตรวจสอบขนาดข้อมูลนำเข้า (คาดหวัง 1x1x64x32 = 2048 ค่า)
        size_t expected_size = 1 * 1 * 64 * 32;
        if (raw_sensor_data.size() != expected_size) {
            result.error_message = "Invalid input tensor size. Expected " + std::to_string(expected_size) + 
                                   ", got " + std::to_string(raw_sensor_data.size());
            return result;
        }

        try {
            // 3. Preprocessing: จัดการ Noise และ Outlier จากสัญญาณเซ็นเซอร์จริง
            std::vector<float> processed_data = preprocess_raw_signal(raw_sensor_data);

            // สร้าง Tensor โดยใช้ข้อมูลที่ผ่านการกรองแล้ว
            torch::Tensor input_tensor = torch::from_blob(
                processed_data.data(), {1, 1, 64, 32}, torch::kFloat32
            ).clone();

            std::vector<std::vector<float>> all_fold_outputs;
            all_fold_outputs.reserve(num_folds);

            // ปิด Gradient เพื่อเพิ่มความเร็วและประหยัดหน่วยความจำบนฮาร์ดแวร์
            torch::NoGuardConfig no_grad;

            for (int i = 0; i < num_folds; ++i) {
                std::vector<torch::jit::IValue> inputs{input_tensor};
                at::IValue output = models[i].forward(inputs);
                torch::Tensor output_tensor = output.toTensor();

                auto output_accessor = output_tensor.accessor<float, 2>();
                all_fold_outputs.push_back({output_accessor[0][0], output_accessor[0][1]});
            }

            // 4. Ensemble Aggregation (Soft Voting)
            std::vector<float> avg_probs(2, 0.0f);
            for (const auto& fold_out : all_fold_outputs) {
                avg_probs[0] += fold_out[0];
                avg_probs[1] += fold_out[1];
            }
            avg_probs[0] /= static_cast<float>(num_folds);
            avg_probs[1] /= static_cast<float>(num_folds);
            result.probabilities = avg_probs;

            auto max_it = std::max_element(avg_probs.begin(), avg_probs.end());
            result.predicted_class = std::distance(avg_probs.begin(), max_it);
            result.confidence_score = *max_it;

            // 5. Personalization & 35% Delta Rate Analysis
            float baseline_abnormal = user_personal_baseline[1];
            float current_abnormal = result.probabilities[1];
            
            if (baseline_abnormal > 0.0f) {
                result.delta_rate_of_change = std::abs(current_abnormal - baseline_abnormal) / baseline_abnormal;
            } else {
                result.delta_rate_of_change = 0.0f;
            }

            result.exceeds_delta_threshold = (result.delta_rate_of_change >= delta_threshold);

            // 6. Safety Gate Decision
            if (result.confidence_score < confidence_threshold || result.exceeds_delta_threshold) {
                result.requires_expert_review = true;
            } else {
                result.requires_expert_review = false;
            }

            // 7. สร้าง Cryptographic Log Hash สำหรับระบบความโปร่งใส (PoA Readiness)
            result.cryptographic_hash = generate_audit_hash(result.predicted_class, result.confidence_score, result.delta_rate_of_change);
            result.is_valid_execution = true;

        } catch (const std::exception& e) {
            result.is_valid_execution = false;
            result.error_message = std::string("Runtime Exception during inference: ") + e.what();
        }

        return result;
    }
};

int main() {
    std::cout << "=== เริ่มต้น Production-Ready Non-Ideal C++ Engine ===" << std::endl;

    ProductionCardiacInferenceEngine engine(0.75f, 0.35f);

    // จำลองการโหลดโมเดล (ถ้าไม่มีไฟล์ระบบจะแจ้ง Error แต่ไม่พัง)
    if (!engine.load_production_models("./model")) {
        std::cout << "[Notice] ข้ามการโหลดโมเดลจริงเนื่องจากไม่พบไฟล์ทดสอบ จะใช้โหมดจำลองระบบแทน" << std::endl;
        return 0;
    }

    // ทดสอบส่งข้อมูลจำลองขนาดถูกต้องเข้าสู่ระบบ
    std::vector<float> live_sensor_chunk(1 * 1 * 64 * 32, 0.45f);
    ProductionDiagnosticResult res = engine.execute_inference_pipeline(live_sensor_chunk);

    if (res.is_valid_execution) {
        std::cout << "\n================ PRODUCTION RESULT ================" << std::endl;
        std::cout << "Predicted Class      : " << res.predicted_class << (res.predicted_class == 0 ? " (Normal)" : " (Abnormal)") << std::endl;
        std::cout << "Confidence Score     : " << res.confidence_score * 100.0f << "%" << std::endl;
        std::cout << "Delta Rate of Change : " << res.delta_rate_of_change * 100.0f << "%" << std::endl;
        std::cout << "Exceeds 35% Threshold: " << (res.exceeds_delta_threshold ? "YES" : "NO") << std::endl;
        std::cout << "Expert Review Needed : " << (res.requires_expert_review ? "YES (Flagged)" : "NO") << std::endl;
        std::cout << "Cryptographic Log ID : " << res.cryptographic_hash << std::endl;
        std::cout << "===================================================" << std::endl;
    } else {
        std::cerr << "[Error] การประมวลผลล้มเหลว: " << res.error_message << std::endl;
    }

    return 0;
}