# 依赖库配置说明

## 需要的第三方库

将以下库放入 `external/` 目录：

### 1. GLFW
- 下载地址: https://www.glfw.org/download.html
- 建议使用源码版本，放入 `external/glfw/`
- 或者使用CMake的FetchContent自动下载

### 2. GLAD
- 生成地址: https://glad.dav1d.de/
- 配置:
  - Language: C/C++
  - Specification: OpenGL
  - API gl: Version 3.3+
  - Profile: Core
- 下载生成的文件，解压到 `external/glad/`
- 目录结构应该是:
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

### 3. GLM
- GitHub: https://github.com/g-truc/glm
- 下载或克隆到 `external/glm/`
- 这是一个header-only库

### 4. ImGui
- GitHub: https://github.com/ocornut/imgui
- 下载或克隆到 `external/imgui/`
- 需要的文件:
  - imgui.h, imgui.cpp
  - imgui_demo.cpp
  - imgui_draw.cpp
  - imgui_tables.cpp
  - imgui_widgets.cpp
  - backends/imgui_impl_glfw.h, imgui_impl_glfw.cpp
  - backends/imgui_impl_opengl3.h, imgui_impl_opengl3.cpp

## 快速设置（推荐）

可以使用Git子模块方式管理依赖：

```bash
# GLFW
git submodule add https://github.com/glfw/glfw.git external/glfw

# GLM
git submodule add https://github.com/g-truc/glm.git external/glm

# ImGui
git submodule add https://github.com/ocornut/imgui.git external/imgui

# 初始化子模块
git submodule update --init --recursive
```

注意：GLAD需要手动从网站生成并添加。
