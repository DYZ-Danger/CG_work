Write-Host "================================================" -ForegroundColor Cyan
Write-Host "Volume Renderer - Dependencies Setup" -ForegroundColor Cyan
Write-Host "================================================" -ForegroundColor Cyan
Write-Host ""

$ErrorActionPreference = "Continue"
$externalDir = Join-Path $PSScriptRoot "external"

if (-not (Test-Path $externalDir)) {
    New-Item -ItemType Directory -Path $externalDir | Out-Null
}

Set-Location $externalDir

# GLFW
Write-Host "[1/3] Checking GLFW..." -ForegroundColor Yellow
$glfwDir = Join-Path $externalDir "glfw"
if (Test-Path (Join-Path $glfwDir "CMakeLists.txt")) {
    Write-Host "  OK: GLFW exists" -ForegroundColor Green
} else {
    Write-Host "  Downloading GLFW..." -ForegroundColor Cyan
    if (Test-Path $glfwDir) { Remove-Item -Path $glfwDir -Recurse -Force }
    try {
        git clone --depth 1 --branch 3.4 https://github.com/glfw/glfw.git glfw
        Write-Host "  OK: GLFW downloaded" -ForegroundColor Green
    } catch {
        Write-Host "  ERROR: GLFW download failed: $_" -ForegroundColor Red
    }
}

# GLM
Write-Host "[2/3] Checking GLM..." -ForegroundColor Yellow
$glmDir = Join-Path $externalDir "glm"
if (Test-Path (Join-Path $glmDir "CMakeLists.txt")) {
    Write-Host "  OK: GLM exists" -ForegroundColor Green
} else {
    Write-Host "  Downloading GLM..." -ForegroundColor Cyan
    if (Test-Path $glmDir) { Remove-Item -Path $glmDir -Recurse -Force }
    try {
        git clone --depth 1 --branch 1.0.1 https://github.com/g-truc/glm.git glm
        Write-Host "  OK: GLM downloaded" -ForegroundColor Green
    } catch {
        Write-Host "  ERROR: GLM download failed: $_" -ForegroundColor Red
    }
}

# ImGui
Write-Host "[3/3] Checking ImGui..." -ForegroundColor Yellow
$imguiDir = Join-Path $externalDir "imgui"
if (Test-Path (Join-Path $imguiDir "imgui.h")) {
    Write-Host "  OK: ImGui exists" -ForegroundColor Green
} else {
    Write-Host "  Downloading ImGui..." -ForegroundColor Cyan
    if (Test-Path $imguiDir) { Remove-Item -Path $imguiDir -Recurse -Force }
    try {
        git clone --depth 1 --branch v1.90.1 https://github.com/ocornut/imgui.git imgui
        Write-Host "  OK: ImGui downloaded" -ForegroundColor Green
    } catch {
        Write-Host "  ERROR: ImGui download failed: $_" -ForegroundColor Red
    }
}

# Verify all dependencies
Write-Host ""
Write-Host "================================================" -ForegroundColor Cyan
Write-Host "Dependency Status:" -ForegroundColor Cyan
Write-Host "================================================" -ForegroundColor Cyan

$allGood = $true

if (Test-Path (Join-Path $externalDir "glad\src\glad.c")) {
    Write-Host "  [OK] GLAD" -ForegroundColor Green
} else {
    Write-Host "  [--] GLAD missing" -ForegroundColor Red
    $allGood = $false
}

if (Test-Path (Join-Path $externalDir "glfw\CMakeLists.txt")) {
    Write-Host "  [OK] GLFW" -ForegroundColor Green
} else {
    Write-Host "  [--] GLFW missing" -ForegroundColor Red
    $allGood = $false
}

if (Test-Path (Join-Path $externalDir "glm\CMakeLists.txt")) {
    Write-Host "  [OK] GLM" -ForegroundColor Green
} else {
    Write-Host "  [--] GLM missing" -ForegroundColor Red
    $allGood = $false
}

if (Test-Path (Join-Path $externalDir "imgui\imgui.h")) {
    Write-Host "  [OK] ImGui" -ForegroundColor Green
} else {
    Write-Host "  [--] ImGui missing" -ForegroundColor Red
    $allGood = $false
}

Write-Host ""
if ($allGood) {
    Write-Host "All dependencies ready!" -ForegroundColor Green
    Write-Host "Next: Run .\rebuild.ps1 to build the project" -ForegroundColor Cyan
} else {
    Write-Host "Some dependencies are missing." -ForegroundColor Red
    Write-Host "See external\README.md for manual setup" -ForegroundColor Yellow
}

Set-Location $PSScriptRoot
Write-Host ""
