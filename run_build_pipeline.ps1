# ====================================================================
# RUN BUILD PIPELINE (Full Production & Multi-Language Support)
# ====================================================================
$ErrorActionPreference = "Stop"

# 1. ค้นหาตำแหน่งของ CMake ในเครื่องอัตโนมัติ (หักดิบหา Full Path)
$CMakePath = (Get-Command cmake.exe -ErrorAction SilentlyContinue).Source
if (-not $CMakePath) {
    # ถ้าใน System Path ไม่เจอ ให้ลองเช็คโฟลเดอร์ติดตั้งมาตรฐาน
    $DefaultPaths = @(
        "C:\Program Files\CMake\bin\cmake.exe",
        "C:\Program Files (x86)\CMake\bin\cmake.exe"
    )
    foreach ($Path in $DefaultPaths) {
        if (Test-Path $Path) { $CMakePath = $Path; break }
    }
}

# บังคับหยุดทำงานทันทีหากเครื่องมือหลักไม่พร้อม
if (-not $CMakePath) {
    Write-Error "🚨 ไม่พบโปรแกรม CMake ในระบบ กรุณาตรวจสอบการติดตั้ง หรือระบุ Path ที่อยู่ของ cmake.exe ในสคริปต์"
    exit 1
}

Write-Host "🔍 Found CMake at: $CMakePath" -ForegroundColor Green

# 2. ตั้งค่า Path สำหรับ LibTorch (ปรับโฟลเดอร์ให้ตรงกับเครื่องของคุณ)
$LibTorchPath = "C:\libtorch" 

# 3. เตรียมโฟลเดอร์สำหรับบิลด์ (Build Directory)
if (-not (Test-Path "build")) {
    New-Item -ItemType Directory -Path "build" | Out-Null
}
Set-Location -Path "build"

# 4. ขั้นตอนการ Configure โปรเจกต์ด้วย CMake
Write-Host "=== [1/2] Running CMake Configuration ===" -ForegroundColor Cyan
# แก้ปัญหาส่งค่าตัวแปรหลุดด้วยการประกาศ Environment Variable แทน
$env:CMAKE_PREFIX_PATH = $LibTorchPath
& $CMakePath -G "Visual Studio 17 2022" -A x64 ..

# 5. ขั้นตอนการคอมไพล์โปรเจกต์ (Build)
Write-Host "=== [2/2] Building Project ===" -ForegroundColor Cyan
& $CMakePath --build . --config Release

Write-Host "✨ Compilation and pipeline build finished successfully!" -ForegroundColor Green
Set-Location -Path ..
