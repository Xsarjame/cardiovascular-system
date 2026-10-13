#include <torch/script.h>
#include <torch/torch.h>
#include <iostream>
#include <vector>
#include <string>
#include <memory>
#include <numeric>
#include <algorithm>
#include <cmath>

// โครงสร้างเก็บผลลัพธ์การวินิจฉัยทางการแพทย์
struct CardiacDiagnosticResult {
    int predicted_class;              // 0 = Normal, 1 = Abnormal / Crisis
    float confidence_score;           // ค่าความมั่นใจ (0.0 - 1.0)
    std::vector<float> probabilities; // ค่าความน่าจะเป็นเฉลี่ยของแต่ละคลาส
    bool requires_expert_review;      // ธงเตือนกรณีเคสก้ำกึ่ง (Low Confidence)
};

// คลาสหลักสำหรับจัดการโมเดลและทำ Ensemble Inference
class CardiacTorchScriptEnsemble {
private:
    std::vector<torch::jit::script::Module> models;
    const int num_folds = 5;
    int num_classes;
    float confidence_threshold;

public:
    CardiacTorchScriptEnsemble(int classes = 2, float threshold = 0.75f) 
        : num_classes(classes), confidence_threshold(threshold) {
        // กำหนดจำนวนเธรดสำหรับการประมวลผลบนซีพียูเพื่อความเร็วสูงสุด
        torch::set_num_threads(2);
    }

    // 1. ฟังก์ชันโหลดโมเดล TorchScript (.pt) ทั้ง 5 โฟลด์
    bool load_models(const std::string& model_dir) {
        try {
            for (int i = 1; i <= num_folds; ++i) {
                std::string model_path = model_dir + "/model_fold_" + std::to_string(i) + "_script.pt";
                
                torch::jit::script::Module module = torch::jit::load(model_path);
                module.eval(); // ตั้งค่าเป็น Evaluation Mode
                
                models.push_back(module);
                std::cout << "[C++ LibTorch] โหลดโมเดลสำเร็จ: " << model_path << std::endl;
            }
            return true;
        } catch (const c10::Error& e) {
            std::cerr << "[C++ Error] ไม่สามารถโหลดโมเดล TorchScript ได้: " << e.what() << std::endl;
            return false;
        }
    }

    // 2. ฟังก์ชันประมวลผล Ensemble Aggregation (Soft Voting + Safety Gate)
    CardiacDiagnosticResult aggregate_predictions(const std::vector<std::vector<float>>& all_fold_outputs) {
        CardiacDiagnosticResult result;
        result.probabilities.assign(num_classes, 0.0f);

        int n_folds = all_fold_outputs.size();
        if (n_folds == 0) return result;

        // คำนวณ Soft Voting (Averaging)
        for (const auto& fold_probs : all_fold_outputs) {
            for (int c = 0; c < num_classes; ++c) {
                result.probabilities[c] += fold_probs[c];
            }
        }

        for (float& prob : result.probabilities) {
            prob /= static_cast<float>(n_folds);
        }

        // หาคลาสที่มีค่าสูงสุด
        auto max_it = std::max_element(result.probabilities.begin(), result.probabilities.end());
        result.predicted_class = std::distance(result.probabilities.begin(), max_it);
        result.confidence_score = *max_it;

        // Safety Gate ตรวจสอบความกำกวมเพื่อส่งต่อแพทย์ผู้เชี่ยวชาญหากจำเป็น
        if (result.confidence_score < confidence_threshold) {
            result.requires_expert_review = true;
            std::cout << "[Ensemble Safety Gate] ⚠️ คำเตือน: ค่าความมั่นใจต่ำกว่าเกณฑ์ (" 
                      << result.confidence_score * 100.0f << "%) -> ส่งต่อผู้เชี่ยวชาญพิจารณาซ้ำ" << std::endl;
        } else {
            result.requires_expert_review = false;
        }

        return result;
    }

    // 3. ฟังก์ชันหลักสำหรับรัน Inference ข้าม 5 โฟลด์
    CardiacDiagnosticResult run_ensemble_inference(std::vector<float>& input_tensor_values) {
        if (models.empty()) {
            std::cerr << "[C++ Error] ยังไม่ได้โหลดโมเดล!" << std::endl;
            return {-1, 0.0f, {}, true};
        }

        // แปลงเวกเตอร์ซีพียูเป็น torch::Tensor (Shape: 1, 1, 64, 32)
        torch::Tensor input_tensor = torch::from_blob(
            input_tensor_values.data(), 
            {1, 1, 64, 32}, 
            torch::kFloat32
        ).clone();

        std::vector<std::vector<float>> all_fold_outputs;
        torch::NoGradGuard no_grad; // ปิด Gradient เพื่อเร่งความเร็ว

        // วนลูป รันโมเดลทั้ง 5 โฟลด์
        for (int i = 0; i < num_folds; ++i) {
            std::vector<torch::jit::IValue> inputs;
            inputs.push_back(input_tensor);

            at::IValue output = models[i].forward(inputs);
            torch::Tensor output_tensor = output.toTensor();

            // ดึงผลลัพธ์ Probability ของโฟลด์นี้
            auto output_accessor = output_tensor.accessor<float, 2>();
            std::vector<float> fold_probs(num_classes);
            for (int c = 0; c < num_classes; ++c) {
                fold_probs[c] = output_accessor[0][c];
            }
            all_fold_outputs.push_back(fold_probs);
        }

        // ส่งผลลัพธ์ทั้งหมดเข้าสู่ระบบรวมผลลัพธ์ (Aggregator & Safety Gate)
        return aggregate_predictions(all_fold_outputs);
    }
};

// ==========================================
// Main Entry Point สำหรับคอมไพล์เป็นโปรแกรมรัน
// ==========================================
int main() {
    std::cout << "=== เริ่มต้น C++ LibTorch Ensemble Inference Engine ===" << std::endl;

    // สร้างอ็อบเจกต์ Engine (กำหนด 2 คลาส, เกณฑ์ความมั่นใจ 75%)
    CardiacTorchScriptEnsemble engine(2, 0.75f);

    // 1. โหลดโมเดลจากโฟลเดอร์ ./model
    if (!engine.load_models("./model")) {
        std::cerr << "การโหลดโมเดลล้มเหลว กรุณาตรวจสอบเส้นทางไฟล์" << std::endl;
        return -1;
    }

    // 2. จำลองข้อมูล Input Tensor ที่ผ่าน Gammatone Pipeline แล้ว (ขนาด 1x1x64x32)
    std::vector<float> dummy_chunk(1 * 1 * 64 * 32, 0.5f);

    // 3. รัน Ensemble Inference
    CardiacDiagnosticResult diagnostic = engine.run_ensemble_inference(dummy_chunk);

    // 4. แสดงผลลัพธ์การวินิจฉัย
    if (diagnostic.predicted_class != -1) {
        std::cout << "\n================ RESULT ================" << std::endl;
        std::cout << "Predicted Class     : " << diagnostic.predicted_class << (diagnostic.predicted_class == 0 ? " (Normal)" : " (Abnormal / Crisis)") << std::endl;
        std::cout << "Confidence Score    : " << diagnostic.confidence_score * 100.0f << "%" << std::endl;
        std::cout << "Expert Review Needed: " << (diagnostic.requires_expert_review ? "YES (Flagged)" : "NO (Safe)") << std::endl;
        std::cout << "========================================" << std::endl;
    }

    return 0;
}