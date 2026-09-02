# 五子棋项目快速构建脚本
# 使用方法：在 PowerShell 中运行 .\build.ps1

Write-Host "========================================" -ForegroundColor Cyan
Write-Host "  五子棋项目构建脚本" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan
Write-Host ""

# 获取脚本所在目录
$projectRoot = Split-Path -Parent $MyInvocation.MyCommand.Path
$buildDir = Join-Path $projectRoot "build"

# Qt 路径（已检测到）
$qtPath = "D:\Qt\6.11.2\mingw_64"
$qtToolsPath = "D:\Qt\Tools"
$cmakePath = "$qtToolsPath\CMake_64\bin"
$ninjaPath = "$qtToolsPath\Ninja"
$mingwPath = "$qtToolsPath\mingw1310_64\bin"

Write-Host "[OK] Qt 路径：$qtPath" -ForegroundColor Green
Write-Host "[OK] CMake 路径：$cmakePath" -ForegroundColor Green
Write-Host "[OK] Ninja 路径：$ninjaPath" -ForegroundColor Green
Write-Host "[OK] MinGW 路径：$mingwPath" -ForegroundColor Green

# 添加所有工具到 PATH
$env:Path = "$cmakePath;$ninjaPath;$mingwPath;$qtPath\bin;$env:Path"

# 检查工具
Write-Host ""
Write-Host "检查工具..." -ForegroundColor Cyan
Write-Host "[OK] $(cmake --version 2>&1 | Select-Object -First 1)" -ForegroundColor Green
Write-Host "[OK] Ninja version: $(ninja --version 2>&1)" -ForegroundColor Green
Write-Host "[OK] GCC version: $($(& "$mingwPath\g++.exe" --version 2>&1 | Select-Object -First 1))" -ForegroundColor Green

# 创建构建目录
Write-Host ""
Write-Host "创建构建目录..." -ForegroundColor Cyan
if (-not (Test-Path $buildDir)) {
    New-Item -ItemType Directory -Path $buildDir | Out-Null
    Write-Host "[OK] 创建目录：$buildDir" -ForegroundColor Green
} else {
    Write-Host "[INFO] 构建目录已存在" -ForegroundColor Yellow
}

# 进入构建目录
Set-Location $buildDir

# 清理旧的构建文件
if (Test-Path "CMakeCache.txt") {
    Write-Host "清理旧的构建文件..." -ForegroundColor Cyan
    Remove-Item "CMakeCache.txt" -Force
    Remove-Item "CMakeFiles" -Recurse -Force -ErrorAction SilentlyContinue
    Remove-Item "build.ninja" -Force -ErrorAction SilentlyContinue
}

# 配置 CMake (使用 Ninja)
Write-Host ""
Write-Host "配置 CMake (使用 Ninja 生成器)..." -ForegroundColor Cyan
$cmakeArgs = @(
    "-G", "Ninja",
    "-DCMAKE_PREFIX_PATH=$qtPath",
    "-DCMAKE_BUILD_TYPE=Release",
    "-DCMAKE_CXX_COMPILER=$mingwPath\g++.exe",
    ".."
)

Write-Host "执行：cmake $($cmakeArgs -join ' ')" -ForegroundColor Gray
cmake @cmakeArgs

if ($LASTEXITCODE -ne 0) {
    Write-Host "[ERROR] CMake 配置失败！" -ForegroundColor Red
    exit 1
}

Write-Host "[OK] CMake 配置成功" -ForegroundColor Green

# 编译项目
Write-Host ""
Write-Host "开始编译..." -ForegroundColor Cyan
cmake --build . --config Release

if ($LASTEXITCODE -ne 0) {
    Write-Host "[ERROR] 编译失败！" -ForegroundColor Red
    Write-Host ""
    Write-Host "请检查上方具体的编译错误信息。" -ForegroundColor Yellow
    exit 1
}

Write-Host "[OK] 编译成功！" -ForegroundColor Green

# 查找可执行文件
$exeName = "Gomoku.exe"
$exePath = Join-Path $buildDir $exeName

if (-not (Test-Path $exePath)) {
    Write-Host "[ERROR] 未找到可执行文件 $exePath" -ForegroundColor Red
    Get-ChildItem $buildDir -Filter "*.exe" | ForEach-Object { Write-Host "找到：$($_.FullName)" }
    exit 1
}

# 部署 Qt DLL
Write-Host ""
Write-Host "部署 Qt 运行时..." -ForegroundColor Cyan
$windeployqt = Join-Path $qtPath "bin\windeployqt.exe"

if (Test-Path $windeployqt) {
    Write-Host "运行 windeployqt..." -ForegroundColor Gray
    & $windeployqt --release $exePath
    Write-Host "[OK] Qt 运行时部署完成" -ForegroundColor Green
} else {
    Write-Host "[WARNING] 未找到 windeployqt" -ForegroundColor Yellow
}

# 完成
Write-Host ""
Write-Host "========================================" -ForegroundColor Cyan
Write-Host "  构建完成！" -ForegroundColor Green
Write-Host "========================================" -ForegroundColor Cyan
Write-Host ""
Write-Host "可执行文件位置：" -ForegroundColor Cyan
Write-Host "  $exePath" -ForegroundColor White
Write-Host ""
Write-Host "运行程序：" -ForegroundColor Cyan
Write-Host "  .\Gomoku.exe" -ForegroundColor White
Write-Host ""
Write-Host "或在 Qt Creator 中打开项目进行调试。" -ForegroundColor Cyan
Write-Host ""
