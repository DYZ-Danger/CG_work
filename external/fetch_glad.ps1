# PowerShell script to download GLAD (OpenGL 3.3 Core)
# Usage: run from repo root or external/ directory
#   pwsh ./external/fetch_glad.ps1

$ErrorActionPreference = "Stop"

$targetDir = Join-Path $PSScriptRoot "glad"
if (-not (Test-Path $targetDir)) {
    New-Item -ItemType Directory -Path $targetDir | Out-Null
}

$zipPath = Join-Path $targetDir "glad.zip"
$extractPath = $targetDir

# Request body for GLAD generator (URL-encoded)
$body = "language=c&specification=gl&api=gl%3D3.3&profile=core&loader=on&extensions="

$uri = 'https://glad.dav1d.de/generate'
Write-Host "Downloading GLAD (OpenGL 3.3 Core) ..."
Invoke-WebRequest -Method Post -Uri $uri -Body $body -ContentType 'application/x-www-form-urlencoded' -OutFile $zipPath

Write-Host "Extracting glad.zip ..."
Expand-Archive -Path $zipPath -DestinationPath $extractPath -Force

# Move extracted include/src to final layout if needed
$generatedRoot = Join-Path $extractPath "glad"
if (Test-Path $generatedRoot) {
    # glad/include -> external/glad/include
    if (Test-Path (Join-Path $generatedRoot 'include')) {
        Copy-Item -Recurse -Force (Join-Path $generatedRoot 'include') $extractPath
    }
    # glad/src -> external/glad/src
    if (Test-Path (Join-Path $generatedRoot 'src')) {
        Copy-Item -Recurse -Force (Join-Path $generatedRoot 'src') $extractPath
    }
}

Remove-Item $zipPath -Force
Write-Host "Done. GLAD placed in: $targetDir"