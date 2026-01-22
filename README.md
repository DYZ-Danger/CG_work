# Volume Renderer - 实时体渲染项目

基于Ray Marching算法的实时体渲染框架，用于图形学大作业。

## 项目概述

本项目实现了一个完整的实时体渲染系统，支持云、烟雾等半透明参与介质的渲染。采用Ray Marching算法，支持多种实时优化技术。

### 主要特性

- ✅ **Ray Marching核心算法** - 沿光线步进采样体积数据
- ✅ **实时渲染** - 基于OpenGL的高性能渲染管线
- ✅ **抖动采样优化** - 减少条带伪影
- ✅ **传输函数** - 可自定义的颜色映射
- ✅ **光照计算** - 基于梯度的法线计算和Phong光照模型
- ✅ **交互式摄像机** - 支持自由移动和旋转
- ✅ **ImGui参数调节** - 实时调整渲染参数

## 技术栈

- **语言**: C++17
- **图形API**: OpenGL 3.3+
- **依赖库**:
  - GLFW - 窗口和输入管理
  - GLAD - OpenGL函数加载
  - GLM - 数学库
  - ImGui - 用户界面

## 项目结构

```
CG_work/
├── assets/            #工具
│   └── VdbToRaw.cpp   # Vdb转换为raw数据（可选）
├── include/           # 头文件
│   ├── Types.h        # 数据结构定义
│   ├── Shader.h       # Shader管理类
│   ├── Camera.h       # 摄像机控制
│   ├── VolumeData.h   # 体数据管理
│   └── Renderer.h     # 渲染器（API接口实现）
├── src/               # 源文件
│   ├── main.cpp       # 主程序入口
│   ├── Shader.cpp
│   ├── Camera.cpp
│   ├── VolumeData.cpp
│   └── Renderer.cpp
├── shaders/           # GLSL着色器
│   ├── raymarching.vert
│   └── raymarching.frag
├── data/              # 体数据文件（可选）
├── external/          # 第三方库（需要手动配置）
│   ├── glfw/
│   ├── glad/
│   ├── glm/
│   └── imgui/
├── CMakeLists.txt     # CMake构建配置
├── rebuild.ps1        # rebuild脚本（Windows）
└── API对接.md         # API接口文档
```

## 编译与运行

### 前置要求

1. **CMake** (>= 3.15)
2. **C++17编译器** (MSVC 2019+, GCC 7+, Clang 5+)
3. **OpenGL 3.3+支持**

### 依赖库安装

在编译前需要将以下库放入 `external/` 目录：

1. **GLFW** - https://www.glfw.org/
2. **GLAD** - https://glad.dav1d.de/ (OpenGL 3.3, Core Profile)
3. **GLM** - https://github.com/g-truc/glm
4. **ImGui** - https://github.com/ocornut/imgui
5. **stb_image**
### 构建步骤

#### Windows (Visual Studio)

```powershell
# 创建构建目录
mkdir build
cd build

# 生成Visual Studio项目
cmake ..

# 编译（或者直接打开.sln文件用VS编译）
cmake --build . --config Release

# 运行
.\bin\Release\VolumeRenderer.exe
```

#### Linux

```bash
mkdir build && cd build
cmake ..
make -j4
./bin/VolumeRenderer
```

## 使用说明

### 控制方式

- **W/A/S/D** - 前后左右移动摄像机
- **Space/Ctrl** - 向上/向下移动
- **鼠标右键拖拽** - 旋转视角
- **鼠标滚轮** - 缩放（调整FOV）
- **ESC** - 退出程序

### 参数调节

程序运行时会显示ImGui控制面板，可以实时调整以下参数：

#### Ray Marching参数
- **Step Size** - 步进大小（越小越精细但越慢）
- **Density** - 密度系数（控制云雾浓度）
- **Threshold** - 阈值（过滤低密度区域）
- **Max Steps** - 最大步进次数

#### 光照参数
- **Enable Lighting** - 启用/禁用光照
- **Absorption** - 吸收系数
- **Scattering** - 散射系数
- **Light Direction** - 光源方向

#### 优化选项
- **Enable Jittering** - 抖动采样（减少条带伪影）
- **Enable Multiple Scattering** - 多重散射近似

#### 模型选择
请先选中模型并加载

## API接口说明

本项目严格按照 `API对接.md` 中定义的接口实现，主要包括：

### 核心接口（Renderer类）

```cpp
// 初始化渲染器
bool InitRenderer(int width, int height);

// 渲染一帧
void RenderFrame();

// 更新渲染参数
void SetRenderParams(const RenderParams& params);

// 更新摄像机
void SetCamera(const Camera& camera);

// 更新传输函数
void SetTransferFunction(const std::vector<glm::vec4>& colors);

// 窗口大小变化
void Resize(int width, int height);

// 获取性能统计
RenderStats GetRenderStats() const;
```

详细的API说明请参考 `API对接.md`。

## 算法实现

### Ray Marching核心流程

1. **光线生成** - 从摄像机位置和方向生成屏幕像素对应的光线
2. **AABB求交** - 计算光线与体积包围盒的交点
3. **体积采样** - 沿光线步进，在每个采样点读取体数据
4. **传输函数映射** - 将密度值映射到颜色和透明度
5. **光照计算** - 基于梯度计算法线，应用Phong光照模型
6. **颜色累积** - Front-to-back合成，累积最终颜色

### 优化技术

- **抖动采样（Jittered Sampling）** - 随机偏移起始点，减少条带伪影
- **早期终止** - 当累积透明度接近不透明时提前结束
- **AABB剔除** - 只渲染与包围盒相交的光线

## 扩展方向

项目框架已预留接口，可以进一步实现：

1. **多重散射近似** - 提升云雾的真实感
2. **自适应步长** - 根据密度变化动态调整步长
3. **体数据预处理** - 实现Mipmap或Octree加速结构
4. **更复杂的传输函数** - 多维传输函数编辑器
5. **真实体数据加载** - 支持.raw, .vol等格式

## 注意事项

- 当前版本使用程序化生成的测试数据（体积云）和现有的模型。
- 如需加载真实体数据，请修改 `VolumeData::LoadFromFile` 方法
- 性能受 `stepSize` 和 `maxSteps` 影响较大，建议在质量和性能间平衡
- 建议直接使用现有的模型，VdbToRaw工具的构建比较复杂。
## 参考资料

- [Perlin Hypertexture](https://ohiostate.pressbooks.pub/app/uploads/sites/45/2017/09/perlin-hypertexture.pdf)
- [Real-Time Volume Graphics](http://www.real-time-volume-graphics.org/)
- OpenGL Programming Guide
- GPU Gems - Volume Rendering Techniques

## 许可证

本项目仅用于教学和学习目的。

## 作者

图形学大作业 - 体渲染小组 程翔瑞 丁怡哲
