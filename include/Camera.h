#ifndef CAMERA_H
#define CAMERA_H

#include "Types.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

class CameraController {
public:
    CameraController();
    ~CameraController() = default;
    
    // 获取摄像机数据
    const Camera& GetCamera() const { return camera; }
    Camera& GetCamera() { return camera; }
    
    // 获取视图矩阵
    glm::mat4 GetViewMatrix() const;
    
    // 获取投影矩阵
    glm::mat4 GetProjectionMatrix() const;
    
    // 处理键盘输入
    void ProcessKeyboard(int direction, float deltaTime);
    
    // 处理鼠标移动
    void ProcessMouseMovement(float xoffset, float yoffset, bool constrainPitch = true);
    
    // 处理鼠标滚轮
    void ProcessMouseScroll(float yoffset);
    
    // 更新摄像机向量
    void UpdateCameraVectors();
    
    // 设置宽高比
    void SetAspectRatio(float ratio) { camera.aspectRatio = ratio; }
    
    enum Movement {
        FORWARD,
        BACKWARD,
        LEFT,
        RIGHT,
        UP,
        DOWN
    };
    
private:
    Camera camera;
};

#endif // CAMERA_H
