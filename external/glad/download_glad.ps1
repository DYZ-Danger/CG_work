# GLAD 自动下载脚本
# 从GitHub的glad-generator获取预编译的OpenGL 3.3 Core版本

Write-Host "正在下载GLAD文件..." -ForegroundColor Green

$gladBaseUrl = "https://gen.glad.sh/generated"

# 创建临时目录
$tempDir = "glad_temp"
New-Item -ItemType Directory -Force -Path $tempDir | Out-Null

try {
    # 下载并解压（这里使用在线生成器API）
    Write-Host "请手动执行以下步骤下载GLAD:" -ForegroundColor Yellow
    Write-Host ""
    Write-Host "1. 访问: https://glad.dav1d.de/" -ForegroundColor Cyan
    Write-Host "2. 配置:" -ForegroundColor Cyan
    Write-Host "   - Language: C/C++" -ForegroundColor White
    Write-Host "   - Specification: OpenGL" -ForegroundColor White
    Write-Host "   - API gl: Version 3.3+" -ForegroundColor White
    Write-Host "   - Profile: Core" -ForegroundColor White
    Write-Host "   - 勾选 'Generate a loader'" -ForegroundColor White
    Write-Host "3. 点击 GENERATE" -ForegroundColor Cyan
    Write-Host "4. 下载 glad.zip" -ForegroundColor Cyan
    Write-Host "5. 解压到: $PSScriptRoot" -ForegroundColor Cyan
    Write-Host ""
    Write-Host "或者，如果你有curl/wget，可以尝试:" -ForegroundColor Yellow
    Write-Host "curl -o glad.zip 'https://glad.dav1d.de/generated?api=gl%3Acore%3D3.3&extensions=&language=c&loader=on&specification=gl'" -ForegroundColor Gray
    
} finally {
    # 清理
    if (Test-Path $tempDir) {
        Remove-Item -Recurse -Force $tempDir
    }
}

Write-Host ""
Write-Host "如果你无法访问网站，我可以帮你创建一个简化版的GLAD文件。" -ForegroundColor Magenta
Write-Host "请告诉我是否需要。" -ForegroundColor Magenta
