#ifndef VOLUMEDATA_H
#define VOLUMEDATA_H

#include <glad/glad.h>
#include <string>
#include <vector>

// 体数据类，负责加载和管理3D纹理
class VolumeData {
public:
    VolumeData();
    ~VolumeData();
    
    // 从原始数据文件加载体数据
    bool LoadFromFile(const std::string& filename, int width, int height, int depth);
    
    // 生成程序化体数据（用于测试）
    bool GenerateProceduralData(int width, int height, int depth);
    
    // 加载float32 RAW体数据（VdbToRaw生成，自动归一化）
    bool LoadFloatRaw(const std::string& filename, int width, int height, int depth);
    
    // 绑定3D纹理
    void Bind(GLuint textureUnit = 0) const;
    
    // 获取纹理ID
    GLuint GetTextureID() const { return textureID; }
    
    // 获取体数据尺寸
    int GetWidth() const { return width; }
    int GetHeight() const { return height; }
    int GetDepth() const { return depth; }
    
private:
    GLuint textureID;
    int width, height, depth;
    
    // 创建3D纹理
    bool CreateTexture3D(const std::vector<unsigned char>& data);
};

#endif // VOLUMEDATA_H
