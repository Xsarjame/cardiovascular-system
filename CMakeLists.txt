cmake_minimum_required(VERSION 3.12)
cmake_minimum_required(VERSION 3.12)
project(cardiovascular-system LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# หักดิบชี้ตรงไปยังจุดที่เก็บไฟล์คอนฟิกของ LibTorch (แก้พาธให้ตรงกับเครื่องของคุณ)
# ข้อสังเกต: มักจะซ่อนอยู่ในโฟลเดอร์ย่อย share/cmake/Torch/ 
set(TORCH_DIR "C:/Users/user/Downloads/libtorch/share/cmake/Torch")

find_package(Torch REQUIRED PATHS ${TORCH_DIR} NO_DEFAULT_PATH)

add_executable(cardiovascular_engine 
    cardiac_inference.cpp 
    production_cardiac_engine.cpp 
    main_complete_pipeline.cpp 
    main_production_system.cpp
)
target_link_libraries(cardiovascular_engine PRIVATE ${TORCH_LIBRARIES})
