param(
    [switch]$Clean,
    [switch]$RunOnly
)

$ErrorActionPreference = "Stop"

if ($Clean -or -not (Test-Path "build")) {
    Write-Host "Full rebuild..." -ForegroundColor Yellow
    Remove-Item -Recurse -Force "build" -ErrorAction SilentlyContinue
    cmake -B "build"
    cmake --build "build" --config Release
} elseif (-not $RunOnly) {
    Write-Host "Incremental build..." -ForegroundColor Cyan
    if (-not (Test-Path "build/CMakeCache.txt")) {
        Write-Host "First build, generating config..." -ForegroundColor Yellow
        cmake -B "build"
    }
    cmake --build "build" --config Release
}

Write-Host "Running program..." -ForegroundColor Green
$exePath = "build\bin\Release\VolumeRenderer.exe"
if (-not (Test-Path $exePath)) {
    $exePath = "build\bin\VolumeRenderer.exe"
}
if (Test-Path $exePath) {
    Push-Location (Split-Path $exePath)
    & (Split-Path $exePath -Leaf)
    Pop-Location
} else {
    Write-Host "ERROR: Executable not found" -ForegroundColor Red
    Write-Host "Please check if build was successful" -ForegroundColor Yellow
}
