#include <torch/script.h>
#include <torch/torch.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <cmath>

// ฟังก์ชันสำหรับอ่าน Binary Reference Tensor ที่ export มาจาก Python
std::vector<float> load_reference_tensor(const std::string& filename, size_t expected_size) {
    std::ifstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "[Error] ไม่สามารถเปิดไฟล์ reference: " << filename << std::endl;
        return {};
    }
    std::vector<float> data(expected_size);
    file.read(reinterpret_cast<char*>(data.data()), expected_size * sizeof(float));
    file.close();
    return data;
}

int main() {
    std::cout << "=== เริ่มต้นตรวจสอบ Numerical Parity ระหว่าง Python และ C++ (LibTorch) ===" << std::endl;

    // 1. โหลดโมเดลตัวอย่าง (เช่น Fold 1) มาทดสอบ Parity
    std::string model_path = "./model/model_fold_1_script.pt";
    torch::jit::script::Module model;
    try {
        model = torch::jit::load(model_path);
        model.eval();
    } catch (const c10::Error& e) {
        std::cerr << "[Critical Error] โหลดโมเดลไม่สำเร็จ: " << e.what() << std::endl;
        return -1;
    }

    // 2. สร้าง Dummy Input หรือโหลด Input เดียวกันกับฝั่ง Python (ขนาด 1x1x64x32)
    size_t input_size = 1 * 1 * 64 * 32;
    std::vector<float> input_data(input_size, 0.5f); // ใช้ค่าจำลอง 0.5 เหมือนกันทั้งสองฝั่ง
    torch::Tensor input_tensor = torch::from_blob(input_data.data(), {1, 1, 64, 32}, torch::kFloat32).clone();

    // 3. รัน Inference ฝั่ง C++
    torch::NoGradGuard no_grad;
    std::vector<torch::jit::IValue> inputs = {input_tensor};
    at::IValue output = model.forward(inputs);
    torch::Tensor cpp_output = output.toTensor();

    auto cpp_accessor = cpp_output.accessor<float, 2>();
    std::cout << "\n[C++ Output ผลลัพธ์จาก LibTorch]:" << std::endl;
    std::cout << "Class 0: " << cpp_accessor[0][0] << " | Class 1: " << cpp_accessor[0][1] << std::endl;

    // 4. โหลดผลลัพธ์เทียบเคียงจาก Python (ถ้ามีไฟล์ .bin ที่บันทึกไว้)
    std::vector<float> py_ref = load_reference_tensor("python_reference_tensor.bin", 2);
    if (!py_ref.empty()) {
        std::cout << "\n[Python Reference Output]:" << std::endl;
        std::cout << "Class 0: " << py_ref[0] << " | Class 1: " << py_ref[1] << std::endl;

        // คำนวณหาค่าความคลาดเคลื่อน (Absolute Difference)
        float diff_class0 = std::abs(cpp_accessor[0][0] - py_ref[0]);
        float diff_class1 = std::abs(cpp_accessor[0][1] - py_ref[1]);
        const float tolerance = 1e-5f; // เกณฑ์ความคลาดเคลื่อนที่ยอมรับได้

        std::cout << "\n[Parity Check Result]:" << std::endl;
        std::cout << "ความต่าง Class 0: " << diff_class0 << std::endl;
        std::cout << "ความต่าง Class 1: " << diff_class1 << std::endl;

        if (diff_class0 < tolerance && diff_class1 < tolerance) {
            std::cout << "✅ [PASS] ผ่านการตรวจสอบ Numerical Parity! ค่าตรงกันระดับทศนิยม" << std::endl;
        } else {
            std::cout << "❌ [FAIL] ผลลัพธ์มีความคลาดเคลื่อนเกินเกณฑ์ที่กำหนด" << std::endl;
        }
    } else {
        std::cout << "ℹ️ [Notice] ไม่พบไฟล์ 'python_reference_tensor.bin' ข้ามขั้นตอนเปรียบเทียบเชิงตัวเลขอัตโนมัติ" << std::endl;
    }

    return 0;
}