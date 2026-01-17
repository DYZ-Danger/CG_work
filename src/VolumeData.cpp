#include "VolumeData.h"
#include <iostream>
#include <cmath>
#include <fstream>
#include <algorithm>
#include <glm/glm.hpp>

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

bool VolumeData::LoadFloatRaw(const std::string& filename, int w, int h, int d) {
    this->width = w;
    this->height = h;
    this->depth = d;
    size_t dataSize = width * height * depth;
    std::vector<float> floatData(dataSize);
    std::ifstream file(filename, std::ios::binary);
    std::cout << "Loading float RAW file: " << filename << std::endl;
    if (!file.is_open()) {
        std::cerr << "Failed to open float RAW file: " << filename << std::endl;
        return false;
    }
    file.read(reinterpret_cast<char*>(floatData.data()), dataSize * sizeof(float));
    file.close();
    if (file.gcount() != dataSize * sizeof(float)) {
        std::cerr << "Failed to read complete float RAW data" << std::endl;
        return false;
    }
    // 归一化到0-255
    float minV = floatData[0], maxV = floatData[0];
    for (float v : floatData) {
        if (v < minV) minV = v;
        if (v > maxV) maxV = v;
    }
    if (fabs(maxV - minV) < 1e-6f) maxV = minV + 1.0f;
    std::vector<unsigned char> data(dataSize);
    for (size_t i = 0; i < dataSize; ++i) {
        float norm = (floatData[i] - minV) / (maxV - minV);
        data[i] = static_cast<unsigned char>(glm::clamp(norm, 0.0f, 1.0f) * 255.0f);
    }
    std::cout << "Float RAW data range: [" << minV << ", " << maxV << "]" << std::endl;
    return this->CreateTexture3D(data);
}

bool VolumeData::GenerateProceduralData(int size, int h, int d) {
    width = size;
    height = (h > 0) ? h : size;
    depth = (d > 0) ? d : size;
    
    size_t dataSize = width * height * depth;
    std::vector<unsigned char> data(dataSize);
    
    // ---------- 程序化云体：Worley + FBM + 高度渐变 + 侵蚀 ----------
    auto hash3 = [](int x, int y, int z) {
        uint32_t h = static_cast<uint32_t>(x * 374761393 + y * 668265263 + z * 2147483647);
        h = (h ^ (h >> 13)) * 1274126177u;
        return h ^ (h >> 16);
    };

    auto rand01 = [&](int x, int y, int z) {
        return (hash3(x, y, z) & 0xFFFF) / 65535.0f;
    };

    auto valueNoise = [&](float fx, float fy, float fz) {
        int x0 = static_cast<int>(std::floor(fx));
        int y0 = static_cast<int>(std::floor(fy));
        int z0 = static_cast<int>(std::floor(fz));
        float tx = fx - x0; float ty = fy - y0; float tz = fz - z0;

        auto lerp = [](float a, float b, float t) { return a + (b - a) * t; };
        auto smooth = [](float t) { return t * t * (3.0f - 2.0f * t); };

        float v000 = rand01(x0,     y0,     z0);
        float v100 = rand01(x0 + 1, y0,     z0);
        float v010 = rand01(x0,     y0 + 1, z0);
        float v110 = rand01(x0 + 1, y0 + 1, z0);
        float v001 = rand01(x0,     y0,     z0 + 1);
        float v101 = rand01(x0 + 1, y0,     z0 + 1);
        float v011 = rand01(x0,     y0 + 1, z0 + 1);
        float v111 = rand01(x0 + 1, y0 + 1, z0 + 1);

        float sx = smooth(tx); float sy = smooth(ty); float sz = smooth(tz);
        float ix00 = lerp(v000, v100, sx);
        float ix10 = lerp(v010, v110, sx);
        float ix01 = lerp(v001, v101, sx);
        float ix11 = lerp(v011, v111, sx);
        float iy0 = lerp(ix00, ix10, sy);
        float iy1 = lerp(ix01, ix11, sy);
        return lerp(iy0, iy1, sz);
    };

    auto fbm = [&](float x, float y, float z) {
        float a = 0.5f;
        float f = 1.0f;
        float sum = 0.0f;
        for (int i = 0; i < 4; ++i) { // 4 层足够生成云的低频细节
            sum += a * valueNoise(x * f, y * f, z * f);
            f *= 2.0f;
            a *= 0.5f;
        }
        return sum;
    };

    auto worley = [&](float x, float y, float z, float freq) {
        // 近似 Worley：取周围 27 个 cell 的随机点，找最近距离
        int xi = static_cast<int>(std::floor(x * freq));
        int yi = static_cast<int>(std::floor(y * freq));
        int zi = static_cast<int>(std::floor(z * freq));
        float minDist = 1e9f;
        for (int dz = -1; dz <= 1; ++dz) {
            for (int dy = -1; dy <= 1; ++dy) {
                for (int dx = -1; dx <= 1; ++dx) {
                    int cx = xi + dx;
                    int cy = yi + dy;
                    int cz = zi + dz;
                    // 随机偏移点
                    float px = (cx + rand01(cx, cy, cz)) / freq;
                    float py = (cy + rand01(cx + 17, cy + 31, cz + 13)) / freq;
                    float pz = (cz + rand01(cx + 7, cy + 11, cz + 19)) / freq;
                    float dxp = px - x;
                    float dyp = py - y;
                    float dzp = pz - z;
                    float dist = std::sqrt(dxp * dxp + dyp * dyp + dzp * dzp);
                    minDist = (minDist < dist) ? minDist : dist;
                }
            }
        }
        // 归一化为 0..1，距离越小密度越高
        float maxDist = std::sqrt(3.0f) / freq;
        float ratio = minDist / maxDist;
        return 1.0f - ((ratio < 1.0f) ? ratio : 1.0f);
    };

    auto smoothstep = [](float edge0, float edge1, float x) {
        float t = (x - edge0) / (edge1 - edge0);
        if (t < 0.0f) t = 0.0f;
        if (t > 1.0f) t = 1.0f;
        return t * t * (3.0f - 2.0f * t);
    };

    for (int z = 0; z < depth; z++) {
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                // 归一化坐标
                float nx = (x + 0.5f) / width;
                float ny = (y + 0.5f) / height;
                float nz = (z + 0.5f) / depth;

                // 双层 Domain warp 打散条纹感，增加体块随机性
                float warp1 = fbm(nx * 1.6f, ny * 1.6f, nz * 1.6f) * 0.30f;
                float warp2 = fbm(nx * 4.0f, ny * 4.0f, nz * 4.0f) * 0.14f;
                float wx = nx + warp1 + warp2 * 0.6f;
                float wy = ny + warp1 * 0.6f + warp2 * 0.35f;
                float wz = nz + warp1 * 0.8f + warp2 * 0.45f;

                // Perlin FBM：生成平滑的基础云形（使用 valueNoise 作为 Perlin）
                float perlinBase = fbm(wx * 1.8f, wy * 1.8f, wz * 1.8f);
                perlinBase = smoothstep(0.2f, 0.8f, perlinBase);

                // Worley 生成云块结构，稍微偏胖的轮廓
                float cellular = worley(wx, wy, wz, 5.5f);
                cellular = pow(cellular, 1.1f);

                // 混合策略：Perlin 为主体（0.7），Worley 为块状补充（0.3）
                float baseNoise = perlinBase * 0.7f + cellular * 0.3f;

                // 低频绒毛 + 高频侵蚀，打破大理石感，增加自然纹理
                float detailLow = fbm(wx * 2.5f, wy * 2.5f, wz * 2.5f);
                float detailHigh = fbm(wx * 8.0f, wy * 8.0f, wz * 8.0f);

                // 基础密度：baseNoise（Perlin+Worley混合）+ 低频细节 - 高频侵蚀
                float densityValue = baseNoise * 1.0f + detailLow * 0.38f - detailHigh * 0.48f;
                // 放宽 smoothstep 范围，扩大覆盖率
                densityValue = smoothstep(0.15f, 0.90f, densityValue);

                // 高度渐变：底部稍微变薄，顶部保留体积
                float heightFade = (ny - 0.02f) / 0.95f;
                if (heightFade < 0.0f) heightFade = 0.0f;
                if (heightFade > 1.0f) heightFade = 1.0f;
                heightFade = pow(heightFade, 1.05f);
                densityValue *= heightFade;

                // 边界淡出效果：距离边界越近，密度越低（实现边界稀疏）
                float edgeFadeX = 1.0f - smoothstep(0.85f, 1.0f, std::abs(nx - 0.5f) * 2.0f);
                float edgeFadeY = 1.0f - smoothstep(0.85f, 1.0f, std::abs(ny - 0.5f) * 2.0f);
                float edgeFadeZ = 1.0f - smoothstep(0.85f, 1.0f, std::abs(nz - 0.5f) * 2.0f);
                float edgeFade = edgeFadeX * edgeFadeY * edgeFadeZ;
                
                // 应用边界淡出
                densityValue *= edgeFade;

                // 体素外完全透明（密度低于阈值的设为0）
                if (densityValue < 0.01f) {
                    densityValue = 0.0f;
                }
                
                // 确保密度值在有效范围内
                if (densityValue < 0.0f) densityValue = 0.0f;
                if (densityValue > 1.0f) densityValue = 1.0f;

                int index = x + y * width + z * width * height;
                data[index] = static_cast<unsigned char>(densityValue * 255.0f);
            }
        }
    }
    
    std::cout << "Generated procedural volume data: " << width << "x" << height << "x" << depth << std::endl;
    return CreateTexture3D(data);
}

bool VolumeData::CreateTexture3D(const std::vector<unsigned char>& data) {
    std::cout << "GL_VENDOR = " << glGetString(GL_VENDOR) << std::endl;

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
    
    std::cout << "Binding 3D texture and uploading data..." << std::endl;
    // 上传数据到3D纹理
    glTexImage3D(
        GL_TEXTURE_3D,
        0,
        GL_R8,                  //明确的内部格式
        width, height, depth,
        0,
        GL_RED,
        GL_UNSIGNED_BYTE,
        data.data()
    );
    glBindTexture(GL_TEXTURE_3D, 0);
        std::cout << "Created 3D texture: " << width << "x" << height << "x" << depth << std::endl;    

    return true;
}

void VolumeData::Bind(GLuint textureUnit) const {
    glActiveTexture(GL_TEXTURE0 + textureUnit);
    glBindTexture(GL_TEXTURE_3D, textureID);
}
