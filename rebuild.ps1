# 快速重新构建并运行脚本
param(
    [switch]$Clean,     # -Clean: 完全清理重建
    [switch]$RunOnly    # -RunOnly: 只运行不构建
)

$ErrorActionPreference = "Stop"

if ($Clean) {
    Write-Host "完全清理重建..." -ForegroundColor Yellow
    Remove-Item -Recurse -Force "build" -ErrorAction SilentlyContinue
    cmake -B "build" -G "MinGW Makefiles"
    cmake --build "build" --config Release
} elseif (-not $RunOnly) {
    Write-Host "增量构建..." -ForegroundColor Cyan
    cmake --build "build" --config Release
}

Write-Host "运行程序..." -ForegroundColor Green
Push-Location "build/bin"
.\VolumeRenderer.exe
Pop-Location
