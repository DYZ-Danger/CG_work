#ifndef RENDERER_H
#define RENDERER_H

#include "Types.h"
#include "Shader.h"
#include "VolumeData.h"
#include "Camera.h"
#include <glad/glad.h>
#include <memory>
#include <vector>

// 渲染器类 - 实现API对接文档中的所有接口
class Renderer {
public:
    Renderer();
    ~Renderer();
    
    // ========== API对接接口 ==========
    
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
    
    // 获取渲染统计信息
    RenderStats GetRenderStats() const;
    
    // ========== 辅助接口 ==========
    
    // 获取摄像机控制器（方便外部进行交互）
    CameraController& GetCameraController() { return *cameraController; }
    
    // 加载体数据
    bool LoadVolumeData(const std::string& filename, int width, int height, int depth);
    
    // 加载float32 RAW体数据（VdbToRaw生成，自动归一化）
    bool LoadFloatRawVolume(const std::string& filename, int width, int height, int depth);
    
    // 生成测试用程序化体数据
    bool GenerateTestVolume(int size = 128);
    
private:
    // 内部渲染状态
    int screenWidth, screenHeight;
    RenderParams renderParams;
    RenderStats renderStats;
    
    // OpenGL资源
    std::unique_ptr<Shader> rayMarchingShader;
    std::unique_ptr<VolumeData> volumeData;
    std::unique_ptr<CameraController> cameraController;
    
    GLuint transferFunctionTexture;
    GLuint quadVAO, quadVBO;
    
    // 性能计时
    float lastFrameTime;
    float deltaTime;
    int frameCount;
    float fpsTimer;
    
    // 内部方法
    void CreateFullScreenQuad();
    void CreateTransferFunctionTexture();
    void UpdateTransferFunctionTexture(const std::vector<glm::vec4>& colors);
    void UpdateUniforms();
};

#endif // RENDERER_H
