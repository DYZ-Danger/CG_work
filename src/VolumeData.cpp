#include "VolumeData.h"
#include <iostream>
#include <cmath>
#include <fstream>

VolumeData::VolumeData() : textureID(0), width(0), height(0), depth(0) {}

VolumeData::~VolumeData() {
    if (textureID != 0) {
        glDeleteTextures(1, &textureID);
    }
}

bool VolumeData::LoadFromFile(const std::string& filename, int w, int h, int d) {
    width = w;
    height = h;
    depth = d;
    
    std::ifstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "Failed to open volume data file: " << filename << std::endl;
        return false;
    }
    
    size_t dataSize = width * height * depth;
    std::vector<unsigned char> data(dataSize);
    file.read(reinterpret_cast<char*>(data.data()), dataSize);
    file.close();
    
    if (file.gcount() != dataSize) {
        std::cerr << "Failed to read complete volume data" << std::endl;
        return false;
    }
    
    return CreateTexture3D(data);
}

bool VolumeData::GenerateProceduralData(int size, int h, int d) {
    width = size;
    height = (h > 0) ? h : size;
    depth = (d > 0) ? d : size;
    
    size_t dataSize = width * height * depth;
    std::vector<unsigned char> data(dataSize);
    
    // 生成程序化的3D Perlin噪声风格数据
    // 这里使用简单的球形渐变作为示例
    float centerX = width * 0.5f;
    float centerY = height * 0.5f;
    float centerZ = depth * 0.5f;
    float maxRadius = std::min(std::min(width, height), depth) * 0.4f;
    
    for (int z = 0; z < depth; z++) {
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                float dx = x - centerX;
                float dy = y - centerY;
                float dz = z - centerZ;
                float distance = std::sqrt(dx*dx + dy*dy + dz*dz);
                
                // 创建多层球形云雾效果
                float value = 0.0f;
                
                // 主球体
                if (distance < maxRadius) {
                    value = 1.0f - (distance / maxRadius);
                    value = std::pow(value, 2.0f);
                }
                
                // 添加一些变化（简单的正弦波扰动）
                float noise = std::sin(x * 0.1f) * std::cos(y * 0.1f) * std::sin(z * 0.1f);
                value += noise * 0.2f;
                value = std::max(0.0f, std::min(1.0f, value));
                
                int index = x + y * width + z * width * height;
                data[index] = static_cast<unsigned char>(value * 255);
            }
        }
    }
    
    std::cout << "Generated procedural volume data: " << width << "x" << height << "x" << depth << std::endl;
    return CreateTexture3D(data);
}

bool VolumeData::CreateTexture3D(const std::vector<unsigned char>& data) {
    if (textureID != 0) {
        glDeleteTextures(1, &textureID);
    }
    
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_3D, textureID);
    
    // 设置纹理参数
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    
    // 上传数据到3D纹理
    glTexImage3D(GL_TEXTURE_3D, 0, GL_RED, width, height, depth, 
                 0, GL_RED, GL_UNSIGNED_BYTE, data.data());
    
    glBindTexture(GL_TEXTURE_3D, 0);
    
    std::cout << "Created 3D texture: " << width << "x" << height << "x" << depth << std::endl;
    return true;
}

void VolumeData::Bind(GLuint textureUnit) const {
    glActiveTexture(GL_TEXTURE0 + textureUnit);
    glBindTexture(GL_TEXTURE_3D, textureID);
}
