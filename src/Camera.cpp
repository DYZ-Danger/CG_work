#include "Camera.h"

CameraController::CameraController() {
    UpdateCameraVectors();
}

glm::mat4 CameraController::GetViewMatrix() const {
    return glm::lookAt(camera.position, camera.position + camera.front, camera.up);
}

glm::mat4 CameraController::GetProjectionMatrix() const {
    return glm::perspective(glm::radians(camera.fov), camera.aspectRatio, 
                           camera.nearPlane, camera.farPlane);
}

void CameraController::ProcessKeyboard(int direction, float deltaTime) {
    float velocity = camera.movementSpeed * deltaTime;
    
    switch (direction) {
        case FORWARD:
            camera.position += camera.front * velocity;
            break;
        case BACKWARD:
            camera.position -= camera.front * velocity;
            break;
        case LEFT:
            camera.position -= camera.right * velocity;
            break;
        case RIGHT:
            camera.position += camera.right * velocity;
            break;
        case UP:
            camera.position += camera.up * velocity;
            break;
        case DOWN:
            camera.position -= camera.up * velocity;
            break;
    }
}

void CameraController::ProcessMouseMovement(float xoffset, float yoffset, bool constrainPitch) {
    xoffset *= camera.mouseSensitivity;
    yoffset *= camera.mouseSensitivity;
    
    camera.yaw += xoffset;
    camera.pitch += yoffset;
    
    // 限制俯仰角
    if (constrainPitch) {
        if (camera.pitch > 89.0f)
            camera.pitch = 89.0f;
        if (camera.pitch < -89.0f)
            camera.pitch = -89.0f;
    }
    
    UpdateCameraVectors();
}

void CameraController::ProcessMouseScroll(float yoffset) {
    camera.fov -= yoffset;
    if (camera.fov < 1.0f)
        camera.fov = 1.0f;
    if (camera.fov > 90.0f)
        camera.fov = 90.0f;
}

void CameraController::UpdateCameraVectors() {
    glm::vec3 front;
    front.x = cos(glm::radians(camera.yaw)) * cos(glm::radians(camera.pitch));
    front.y = sin(glm::radians(camera.pitch));
    front.z = sin(glm::radians(camera.yaw)) * cos(glm::radians(camera.pitch));
    camera.front = glm::normalize(front);
    
    // 重新计算右向量和上向量
    camera.right = glm::normalize(glm::cross(camera.front, glm::vec3(0.0f, 1.0f, 0.0f)));
    camera.up = glm::normalize(glm::cross(camera.right, camera.front));
}
