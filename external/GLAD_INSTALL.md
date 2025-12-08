# GLAD 安装说明

GLAD需要从网站手动生成，因为它是根据你的需求定制的。

## 方法1：从网站生成（推荐）

1. 访问：https://glad.dav1d.de/

2. 配置选项：
   - **Language**: C/C++
   - **Specification**: OpenGL
   - **API gl**: Version 3.3 (或更高)
   - **Profile**: Core
   - 勾选 "Generate a loader"

3. 点击 "GENERATE"

4. 下载生成的 `glad.zip` 文件

5. 解压文件到当前目录 (`external/glad/`)
   - 确保目录结构为：
   ```
   external/glad/
   ├── include/
   │   ├── glad/
   │   │   └── glad.h
   │   └── KHR/
   │       └── khrplatform.h
   └── src/
       └── glad.c
   ```

## 方法2：使用脚本自动下载（尝试）

仓库已提供脚本 `external/fetch_glad.ps1`，会向GLAD官网提交生成请求并自动解压：

```powershell
pwsh ./external/fetch_glad.ps1
```

如果脚本因为网络原因失败，请改用方法1手动下载。

## 方法3：使用预生成版本

如果无法访问网站，可以从其它项目拷贝预生成的GLAD 3.3 Core 版本，保持以下结构即可。

## 验证安装

安装完成后，你的 `external` 目录应该包含：
```
external/
├── glfw/      ✓ 已安装
├── glad/      ⚠ 需要手动添加
├── glm/       ✓ 已安装
└── imgui/     ✓ 已安装
```

完成后即可进行CMake构建！
