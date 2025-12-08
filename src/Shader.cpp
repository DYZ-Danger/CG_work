#include "Shader.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <glm/gtc/type_ptr.hpp>

Shader::Shader() : ID(0) {}

Shader::~Shader() {
    if (ID != 0) {
        glDeleteProgram(ID);
    }
}

bool Shader::LoadFromFile(const std::string& vertexPath, const std::string& fragmentPath) {
    // 读取顶点着色器文件
    std::ifstream vShaderFile(vertexPath);
    if (!vShaderFile.is_open()) {
        std::cerr << "Failed to open vertex shader file: " << vertexPath << std::endl;
        return false;
    }
    std::stringstream vShaderStream;
    vShaderStream << vShaderFile.rdbuf();
    std::string vertexCode = vShaderStream.str();
    vShaderFile.close();
    
    // 读取片段着色器文件
    std::ifstream fShaderFile(fragmentPath);
    if (!fShaderFile.is_open()) {
        std::cerr << "Failed to open fragment shader file: " << fragmentPath << std::endl;
        return false;
    }
    std::stringstream fShaderStream;
    fShaderStream << fShaderFile.rdbuf();
    std::string fragmentCode = fShaderStream.str();
    fShaderFile.close();
    
    // 编译着色器
    GLuint vertexShader, fragmentShader;
    if (!CompileShader(vertexCode, GL_VERTEX_SHADER, vertexShader)) {
        return false;
    }
    if (!CompileShader(fragmentCode, GL_FRAGMENT_SHADER, fragmentShader)) {
        glDeleteShader(vertexShader);
        return false;
    }
    
    // 链接程序
    bool success = LinkProgram(vertexShader, fragmentShader);
    
    // 删除着色器（已经链接到程序中）
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    
    return success;
}

bool Shader::CompileShader(const std::string& source, GLenum type, GLuint& shader) {
    shader = glCreateShader(type);
    const char* src = source.c_str();
    glShaderSource(shader, 1, &src, nullptr);
    glCompileShader(shader);
    
    // 检查编译错误
    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        CheckCompileErrors(shader, type == GL_VERTEX_SHADER ? "VERTEX" : "FRAGMENT");
        return false;
    }
    return true;
}

bool Shader::LinkProgram(GLuint vertexShader, GLuint fragmentShader) {
    ID = glCreateProgram();
    glAttachShader(ID, vertexShader);
    glAttachShader(ID, fragmentShader);
    glLinkProgram(ID);
    
    // 检查链接错误
    GLint success;
    glGetProgramiv(ID, GL_LINK_STATUS, &success);
    if (!success) {
        CheckCompileErrors(ID, "PROGRAM");
        return false;
    }
    return true;
}

void Shader::CheckCompileErrors(GLuint shader, const std::string& type) {
    GLint success;
    GLchar infoLog[1024];
    if (type != "PROGRAM") {
        glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
        if (!success) {
            glGetShaderInfoLog(shader, 1024, nullptr, infoLog);
            std::cerr << "ERROR::SHADER_COMPILATION_ERROR of type: " << type << "\n" 
                     << infoLog << std::endl;
        }
    } else {
        glGetProgramiv(shader, GL_LINK_STATUS, &success);
        if (!success) {
            glGetProgramInfoLog(shader, 1024, nullptr, infoLog);
            std::cerr << "ERROR::PROGRAM_LINKING_ERROR of type: " << type << "\n" 
                     << infoLog << std::endl;
        }
    }
}

void Shader::Use() const {
    glUseProgram(ID);
}

void Shader::SetBool(const std::string& name, bool value) const {
    glUniform1i(glGetUniformLocation(ID, name.c_str()), (int)value);
}

void Shader::SetInt(const std::string& name, int value) const {
    glUniform1i(glGetUniformLocation(ID, name.c_str()), value);
}

void Shader::SetFloat(const std::string& name, float value) const {
    glUniform1f(glGetUniformLocation(ID, name.c_str()), value);
}

void Shader::SetVec3(const std::string& name, const glm::vec3& value) const {
    glUniform3fv(glGetUniformLocation(ID, name.c_str()), 1, glm::value_ptr(value));
}

void Shader::SetVec4(const std::string& name, const glm::vec4& value) const {
    glUniform4fv(glGetUniformLocation(ID, name.c_str()), 1, glm::value_ptr(value));
}

void Shader::SetMat4(const std::string& name, const glm::mat4& mat) const {
    glUniformMatrix4fv(glGetUniformLocation(ID, name.c_str()), 1, GL_FALSE, glm::value_ptr(mat));
}
