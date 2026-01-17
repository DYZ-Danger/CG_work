#include "Renderer.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <iostream>
#include <fstream>
#include <sstream>

// 全局变量
Renderer* g_renderer = nullptr;
GLFWwindow* g_window = nullptr;
bool g_firstMouse = true;
float g_lastX = 400.0f;
float g_lastY = 300.0f;
bool g_mousePressed = false;

// GLFW回调函数
void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    if (g_renderer) {
        g_renderer->Resize(width, height);
    }
}

void mouse_callback(GLFWwindow* window, double xpos, double ypos) {
    if (!g_mousePressed) return;
    
    if (g_firstMouse) {
        g_lastX = (float)xpos;
        g_lastY = (float)ypos;
        g_firstMouse = false;
    }
    
    float xoffset = (float)xpos - g_lastX;
    float yoffset = g_lastY - (float)ypos;
    g_lastX = (float)xpos;
    g_lastY = (float)ypos;
    
    if (g_renderer) {
        g_renderer->GetCameraController().ProcessMouseMovement(xoffset, yoffset);
    }
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
    if (button == GLFW_MOUSE_BUTTON_RIGHT) {
        if (action == GLFW_PRESS) {
            g_mousePressed = true;
            g_firstMouse = true;
        } else if (action == GLFW_RELEASE) {
            g_mousePressed = false;
        }
    }
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
    if (g_renderer) {
        g_renderer->GetCameraController().ProcessMouseScroll((float)yoffset);
    }
}

void processInput(GLFWwindow* window, float deltaTime) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, true);
    }
    
    if (!g_renderer) return;
    
    auto& camController = g_renderer->GetCameraController();
    
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        camController.ProcessKeyboard(CameraController::FORWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        camController.ProcessKeyboard(CameraController::BACKWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        camController.ProcessKeyboard(CameraController::LEFT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        camController.ProcessKeyboard(CameraController::RIGHT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
        camController.ProcessKeyboard(CameraController::UP, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS)
        camController.ProcessKeyboard(CameraController::DOWN, deltaTime);
}

bool InitGLFW() {
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return false;
    }
    
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    
    g_window = glfwCreateWindow(1280, 720, "Volume Renderer - Ray Marching", nullptr, nullptr);
    if (!g_window) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return false;
    }
    
    glfwMakeContextCurrent(g_window);
    glfwSetFramebufferSizeCallback(g_window, framebuffer_size_callback);
    glfwSetCursorPosCallback(g_window, mouse_callback);
    glfwSetMouseButtonCallback(g_window, mouse_button_callback);
    glfwSetScrollCallback(g_window, scroll_callback);
    
    return true;
}

bool InitGLAD() {
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "Failed to initialize GLAD" << std::endl;
        return false;
    }
    return true;
}

void InitImGui() {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    
    ImGui::StyleColorsDark();
    
    ImGui_ImplGlfw_InitForOpenGL(g_window, true);
    ImGui_ImplOpenGL3_Init("#version 330");
}

void RenderImGui(RenderParams& params) {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    
    // 控制面板
    ImGui::Begin("Volume Rendering Controls");
    
    ImGui::Text("Performance");
    RenderStats stats = g_renderer->GetRenderStats();
    ImGui::Text("FPS: %.1f", stats.fps);
    ImGui::Text("Frame Time: %.2f ms", stats.frameTimeMs);
    
    ImGui::Separator();
    ImGui::Text("Ray Marching Parameters");
    ImGui::SliderFloat("Step Size", &params.stepSize, 0.001f, 0.1f, "%.4f");
    ImGui::SliderFloat("Density", &params.density, 0.0f, 5.0f);
    ImGui::SliderFloat("Threshold", &params.threshold, 0.0f, 1.0f);
    ImGui::SliderInt("Max Steps", &params.maxSteps, 64, 512);
    
    ImGui::Separator();
    ImGui::Text("Lighting");
    ImGui::Checkbox("Enable Lighting", &params.enableLighting);
    ImGui::SliderFloat("Absorption", &params.absorptionCoeff, 0.0f, 5.0f);
    ImGui::SliderFloat("Scattering", &params.scatteringCoeff, 0.0f, 2.0f);
    ImGui::SliderFloat3("Light Direction", &params.lightDir.x, -1.0f, 1.0f);
    
    ImGui::Separator();
    ImGui::Text("Optimizations");
    ImGui::Checkbox("Enable Jittering", &params.enableJittering);

    ImGui::Separator();
    ImGui::Text("Translucency");
    ImGui::SliderFloat("Translucency", &params.translucency, 0.0f, 1.0f);
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("0=厚重  1=通透(更少Alpha、更浅阴影)");
    }
    
    ImGui::Separator();
    ImGui::Text("Camera Controls");
    ImGui::Text("WASD - Move");
    ImGui::Text("Space/Ctrl - Up/Down");
    ImGui::Text("Right Mouse - Look Around");
    ImGui::Text("Scroll - Zoom");
    
    ImGui::End();
    
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

int main() {
    // 初始化GLFW
    if (!InitGLFW()) {
        return -1;
    }
    
    // 初始化GLAD
    if (!InitGLAD()) {
        glfwTerminate();
        return -1;
    }
    
    // 初始化ImGui
    InitImGui();
    
    // 创建渲染器
    g_renderer = new Renderer();
    if (!g_renderer->InitRenderer(1280, 720)) {
        std::cerr << "Failed to initialize renderer" << std::endl;
        delete g_renderer;
        glfwTerminate();
        return -1;
    }
    
    std::cout << "\n==================================" << std::endl;
    std::cout << "Volume Renderer Initialized!" << std::endl;
    std::cout << "==================================" << std::endl;
    std::cout << "Controls:" << std::endl;
    std::cout << "  WASD - Move camera" << std::endl;
    std::cout << "  Space/Ctrl - Move up/down" << std::endl;
    std::cout << "  Right Mouse - Look around" << std::endl;
    std::cout << "  Scroll - Zoom in/out" << std::endl;
    std::cout << "  ESC - Exit" << std::endl;
    
    // 渲染参数
    RenderParams params;

    // 自动读取RAW体数据分辨率
    /*int rawWidth = 0, rawHeight = 0, rawDepth = 0;
    {
        std::ifstream metaFile("data/smoke.raw.meta");
        if (metaFile) {
            std::string line;
            if (std::getline(metaFile, line)) {
                std::istringstream iss(line);
                iss >> rawWidth >> rawHeight >> rawDepth;
            }
        }
    }
    if (rawWidth > 0 && rawHeight > 0 && rawDepth > 0) {
        g_renderer->LoadFloatRawVolume("data/smoke.raw", rawWidth, rawHeight, rawDepth);
    } else {
        std::cerr << "Failed to read RAW volume resolution from meta file!" << std::endl;
    }*/
    
    // 主循环
    float lastFrameTime = 0.0f;
    while (!glfwWindowShouldClose(g_window)) {
        // 计算deltaTime
        float currentTime = (float)glfwGetTime();
        float deltaTime = currentTime - lastFrameTime;
        lastFrameTime = currentTime;
        
        // 处理输入
        processInput(g_window, deltaTime);
        
        // 更新渲染参数
        g_renderer->SetRenderParams(params);
        
        // 渲染
        g_renderer->RenderFrame();
        
        // 渲染UI
        RenderImGui(params);
        
        // 交换缓冲区和轮询事件
        glfwSwapBuffers(g_window);
        glfwPollEvents();
    }
    
    // 清理
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    
    delete g_renderer;
    glfwDestroyWindow(g_window);
    glfwTerminate();
    
    return 0;
}
