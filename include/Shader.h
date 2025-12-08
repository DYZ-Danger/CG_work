#ifndef SHADER_H
#define SHADER_H

#include <string>
#include <glad/glad.h>
#include <glm/glm.hpp>

class Shader {
public:
    GLuint ID;
    
    Shader();
    ~Shader();
    
    // 从文件加载并编译shader
    bool LoadFromFile(const std::string& vertexPath, const std::string& fragmentPath);
    
    // 使用shader程序
    void Use() const;
    
    // Uniform工具函数
    void SetBool(const std::string& name, bool value) const;
    void SetInt(const std::string& name, int value) const;
    void SetFloat(const std::string& name, float value) const;
    void SetVec3(const std::string& name, const glm::vec3& value) const;
    void SetVec4(const std::string& name, const glm::vec4& value) const;
    void SetMat4(const std::string& name, const glm::mat4& mat) const;
    
private:
    // 编译shader
    bool CompileShader(const std::string& source, GLenum type, GLuint& shader);
    // 链接程序
    bool LinkProgram(GLuint vertexShader, GLuint fragmentShader);
    // 检查编译/链接错误
    void CheckCompileErrors(GLuint shader, const std::string& type);
};

#endif // SHADER_H
