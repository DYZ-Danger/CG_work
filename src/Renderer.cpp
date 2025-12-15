#include "Renderer.h"
#include <GLFW/glfw3.h>
#include <iostream>
#include <glm/gtc/matrix_transform.hpp>

Renderer::Renderer() 
    : screenWidth(800), screenHeight(600),
      transferFunctionTexture(0), quadVAO(0), quadVBO(0),
      lastFrameTime(0.0f), deltaTime(0.0f), frameCount(0), fpsTimer(0.0f) {
}

Renderer::~Renderer() {
    if (transferFunctionTexture != 0) {
        glDeleteTextures(1, &transferFunctionTexture);
    }
    if (quadVAO != 0) {
        glDeleteVertexArrays(1, &quadVAO);
    }
    if (quadVBO != 0) {
        glDeleteBuffers(1, &quadVBO);
    }
}

bool Renderer::InitRenderer(int width, int height) {
    screenWidth = width;
    screenHeight = height;
    
    std::cout << "Initializing Renderer..." << std::endl;
    
    // 创建摄像机控制器
    cameraController = std::make_unique<CameraController>();
    cameraController->SetAspectRatio((float)width / (float)height);
    
    // 加载Ray Marching Shader
    rayMarchingShader = std::make_unique<Shader>();
    if (!rayMarchingShader->LoadFromFile("shaders/raymarching.vert", "shaders/raymarching.frag")) {
        std::cerr << "Failed to load ray marching shaders" << std::endl;
        return false;
    }
    
    // 创建全屏四边形
    CreateFullScreenQuad();
    
    // 创建传输函数纹理
    CreateTransferFunctionTexture();
    
    // 生成测试体数据
    if (!GenerateTestVolume(128)) {
        std::cerr << "Failed to generate test volume" << std::endl;
        return false;
    }
    
    // 初始化性能计时
    lastFrameTime = (float)glfwGetTime();
    
    std::cout << "Renderer initialized successfully" << std::endl;
    return true;
}

void Renderer::RenderFrame() {
    // 计算帧时间
    float currentTime = (float)glfwGetTime();
    deltaTime = currentTime - lastFrameTime;
    lastFrameTime = currentTime;
    
    // 更新FPS统计
    frameCount++;
    fpsTimer += deltaTime;
    if (fpsTimer >= 1.0f) {
        renderStats.fps = frameCount / fpsTimer;
        renderStats.frameTimeMs = (fpsTimer / frameCount) * 1000.0f;
        frameCount = 0;
        fpsTimer = 0.0f;
    }
    
    // 清屏
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    // 使用Ray Marching shader
    rayMarchingShader->Use();
    
    // 更新uniform变量
    UpdateUniforms();
    
    // 绑定体数据纹理
    if (volumeData) {
        volumeData->Bind(0);
    }
    
    // 绑定传输函数纹理
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_1D, transferFunctionTexture);
    
    // 渲染全屏四边形
    glBindVertexArray(quadVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
}

void Renderer::SetRenderParams(const RenderParams& params) {
    renderParams = params;
}

void Renderer::SetCamera(const Camera& camera) {
    cameraController->GetCamera() = camera;
}

void Renderer::SetTransferFunction(const std::vector<glm::vec4>& colors) {
    UpdateTransferFunctionTexture(colors);
}

void Renderer::Resize(int width, int height) {
    screenWidth = width;
    screenHeight = height;
    glViewport(0, 0, width, height);
    cameraController->SetAspectRatio((float)width / (float)height);
}

RenderStats Renderer::GetRenderStats() const {
    return renderStats;
}

bool Renderer::LoadVolumeData(const std::string& filename, int width, int height, int depth) {
    volumeData = std::make_unique<VolumeData>();
    return volumeData->LoadFromFile(filename, width, height, depth);
}

bool Renderer::GenerateTestVolume(int size) {
    volumeData = std::make_unique<VolumeData>();
    return volumeData->GenerateProceduralData(size, size, size);
}

void Renderer::CreateFullScreenQuad() {
    float quadVertices[] = {
        // 位置            // 纹理坐标
        -1.0f,  1.0f, 0.0f,  0.0f, 1.0f,
        -1.0f, -1.0f, 0.0f,  0.0f, 0.0f,
         1.0f, -1.0f, 0.0f,  1.0f, 0.0f,
        
        -1.0f,  1.0f, 0.0f,  0.0f, 1.0f,
         1.0f, -1.0f, 0.0f,  1.0f, 0.0f,
         1.0f,  1.0f, 0.0f,  1.0f, 1.0f
    };
    
    glGenVertexArrays(1, &quadVAO);
    glGenBuffers(1, &quadVBO);
    
    glBindVertexArray(quadVAO);
    glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);
    
    // 位置属性
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    
    // 纹理坐标属性
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    
    glBindVertexArray(0);
}

void Renderer::CreateTransferFunctionTexture() {
    // 创建默认的传输函数（线性渐变）
    const int tfSize = 256;
    std::vector<glm::vec4> tfData(tfSize);
    
    for (int i = 0; i < tfSize; i++) {
        float t = (float)i / (tfSize - 1);
        // 更亮、更透明的云：低密度保持亮度，透明度略放缓
        float alpha = pow(t, 0.85f) * 0.7f;
        glm::vec3 base = glm::mix(glm::vec3(0.82f, 0.86f, 0.95f), glm::vec3(0.98f, 0.99f, 1.0f), t);
        tfData[i] = glm::vec4(base, alpha);
    }
    
    glGenTextures(1, &transferFunctionTexture);
    glBindTexture(GL_TEXTURE_1D, transferFunctionTexture);
    
    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    
    glTexImage1D(GL_TEXTURE_1D, 0, GL_RGBA, tfSize, 0, GL_RGBA, GL_FLOAT, tfData.data());
    
    glBindTexture(GL_TEXTURE_1D, 0);
}

void Renderer::UpdateTransferFunctionTexture(const std::vector<glm::vec4>& colors) {
    if (colors.empty()) return;
    
    glBindTexture(GL_TEXTURE_1D, transferFunctionTexture);
    glTexImage1D(GL_TEXTURE_1D, 0, GL_RGBA, colors.size(), 0, GL_RGBA, GL_FLOAT, colors.data());
    glBindTexture(GL_TEXTURE_1D, 0);
}

void Renderer::UpdateUniforms() {
    // 设置纹理单元
    rayMarchingShader->SetInt("volumeTexture", 0);
    rayMarchingShader->SetInt("transferFunction", 1);
    
    // 设置渲染参数
    rayMarchingShader->SetFloat("stepSize", renderParams.stepSize);
    rayMarchingShader->SetFloat("density", renderParams.density);
    rayMarchingShader->SetFloat("threshold", renderParams.threshold);
    rayMarchingShader->SetBool("enableLighting", renderParams.enableLighting);
    rayMarchingShader->SetFloat("absorptionCoeff", renderParams.absorptionCoeff);
    rayMarchingShader->SetFloat("scatteringCoeff", renderParams.scatteringCoeff);
    rayMarchingShader->SetVec3("lightDir", glm::normalize(renderParams.lightDir));
    rayMarchingShader->SetInt("maxSteps", renderParams.maxSteps);
    rayMarchingShader->SetBool("enableJittering", renderParams.enableJittering);

    // 通透度联动映射
    float t = glm::clamp(renderParams.translucency, 0.0f, 1.0f);
    float alphaScale = glm::mix(0.9f, 0.6f, t);      // 越通透，整体Alpha越小
    float shadowMin  = glm::mix(0.88f, 0.95f, t);    // 越通透，阴影最暗值越高
    float shadowAtten= glm::mix(1.0f, 0.75f, t);     // 越通透，阴影衰减越弱
    rayMarchingShader->SetFloat("alphaScale", alphaScale);
    rayMarchingShader->SetFloat("shadowMin", shadowMin);
    rayMarchingShader->SetFloat("shadowAtten", shadowAtten);
    
    // 设置摄像机矩阵
    const Camera& cam = cameraController->GetCamera();
    glm::mat4 view = cameraController->GetViewMatrix();
    glm::mat4 projection = cameraController->GetProjectionMatrix();
    glm::mat4 invView = glm::inverse(view);
    glm::mat4 invProjection = glm::inverse(projection);
    
    rayMarchingShader->SetMat4("invView", invView);
    rayMarchingShader->SetMat4("invProjection", invProjection);
    rayMarchingShader->SetVec3("cameraPos", cam.position);
    
    // 设置时间（用于抖动采样）
    rayMarchingShader->SetFloat("time", (float)glfwGetTime());
}
