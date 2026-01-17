#ifndef TYPES_H
#define TYPES_H

#include <glm/glm.hpp>
#include <vector>

// 渲染参数结构体
struct RenderParams {
    float stepSize = 0.01f;          // Ray Marching步长
    float density = 1.0f;             // 密度系数
    float threshold = 0.1f;           // 阈值
    bool enableLighting = true;       // 是否启用光照
    float absorptionCoeff = 1.0f;     // 吸收系数
    float scatteringCoeff = 0.5f;     // 散射系数
    glm::vec3 lightDir = glm::vec3(0.3f, -0.8f, 0.5f);  // 光照方向（从上方偏后照向下）
    int maxSteps = 256;               // 最大步进次数
    bool enableJittering = true;      // 抖动采样优化
    
    // MSAA 参数
    int msaaSamples = 2;              // MSAA 采样数
    float msaaRadius = 0.3f;          // MSAA 采样半径

    // 多重散射估计
    bool enableMultipleScattering = false; // 是否启用多重散射近似
    int multiScatterSteps = 4;              // 近似步数（少量以控制性能）
    float multiScatterStrength = 0.35f;     // 贡献权重

    // 通透度（0=偏厚重，1=偏通透）
    float translucency = 0.8f;
};

// 摄像机结构体
struct Camera {
    glm::vec3 position = glm::vec3(0.0f, 0.0f, 3.0f);
    glm::vec3 front = glm::vec3(0.0f, 0.0f, -1.0f);
    glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);
    glm::vec3 right = glm::vec3(1.0f, 0.0f, 0.0f);
    float fov = 45.0f;
    float aspectRatio = 16.0f / 9.0f;
    float nearPlane = 0.1f;
    float farPlane = 100.0f;
    
    // 欧拉角
    float yaw = -90.0f;
    float pitch = 0.0f;
    
    // 移动速度
    float movementSpeed = 2.5f;
    float mouseSensitivity = 0.1f;
};

// 渲染统计信息
struct RenderStats {
    float fps = 0.0f;
    float frameTimeMs = 0.0f;
    int triangleCount = 0;
};

// 传输函数颜色点
struct TransferFunctionPoint {
    float value;        // [0, 1] 范围的值
    glm::vec4 color;    // RGBA颜色
};

#endif // TYPES_H
