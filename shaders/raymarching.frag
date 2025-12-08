#version 330 core

in vec2 TexCoord;
out vec4 FragColor;

// 纹理
uniform sampler3D volumeTexture;
uniform sampler1D transferFunction;

// 渲染参数
uniform float stepSize;
uniform float density;
uniform float threshold;
uniform bool enableLighting;
uniform float absorptionCoeff;
uniform float scatteringCoeff;
uniform vec3 lightDir;
uniform int maxSteps;
uniform bool enableJittering;

// 摄像机
uniform mat4 invView;
uniform mat4 invProjection;
uniform vec3 cameraPos;
uniform float time;

// 体积包围盒
const vec3 boxMin = vec3(-0.5);
const vec3 boxMax = vec3(0.5);

// 随机函数（用于抖动采样）
float random(vec2 st) {
    return fract(sin(dot(st.xy, vec2(12.9898, 78.233))) * 43758.5453123);
}

// 计算光线与AABB的交点
bool intersectAABB(vec3 rayOrigin, vec3 rayDir, vec3 boxMin, vec3 boxMax, out float tNear, out float tFar) {
    vec3 invDir = 1.0 / rayDir;
    vec3 t0 = (boxMin - rayOrigin) * invDir;
    vec3 t1 = (boxMax - rayOrigin) * invDir;
    
    vec3 tmin = min(t0, t1);
    vec3 tmax = max(t0, t1);
    
    tNear = max(max(tmin.x, tmin.y), tmin.z);
    tFar = min(min(tmax.x, tmax.y), tmax.z);
    
    return tFar > tNear && tFar > 0.0;
}

// 计算梯度（用于光照）
vec3 computeGradient(vec3 pos) {
    float offset = 0.01;
    float dx = texture(volumeTexture, pos + vec3(offset, 0, 0)).r - texture(volumeTexture, pos - vec3(offset, 0, 0)).r;
    float dy = texture(volumeTexture, pos + vec3(0, offset, 0)).r - texture(volumeTexture, pos - vec3(0, offset, 0)).r;
    float dz = texture(volumeTexture, pos + vec3(0, 0, offset)).r - texture(volumeTexture, pos - vec3(0, 0, offset)).r;
    return normalize(vec3(dx, dy, dz));
}

// 简单的光照计算
vec3 computeLighting(vec3 normal, vec3 viewDir, vec3 color) {
    // 环境光
    vec3 ambient = 0.3 * color;
    
    // 漫反射
    float diff = max(dot(normal, lightDir), 0.0);
    vec3 diffuse = diff * color;
    
    // 镜面反射
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32.0);
    vec3 specular = 0.5 * spec * vec3(1.0);
    
    return ambient + diffuse + specular;
}

void main() {
    // 从屏幕空间坐标重建世界空间光线
    vec4 clipPos = vec4(TexCoord * 2.0 - 1.0, -1.0, 1.0);
    vec4 viewPos = invProjection * clipPos;
    viewPos /= viewPos.w;
    
    vec3 rayDir = normalize((invView * vec4(viewPos.xyz, 0.0)).xyz);
    vec3 rayOrigin = cameraPos;
    
    // 计算与体积包围盒的交点
    float tNear, tFar;
    if (!intersectAABB(rayOrigin, rayDir, boxMin, boxMax, tNear, tFar)) {
        FragColor = vec4(0.0, 0.0, 0.0, 1.0);
        return;
    }
    
    // 确保起点在包围盒内
    if (tNear < 0.0) tNear = 0.0;
    
    // Ray Marching初始化
    vec3 startPos = rayOrigin + rayDir * tNear;
    vec3 endPos = rayOrigin + rayDir * tFar;
    float rayLength = distance(startPos, endPos);
    
    // 抖动采样优化（减少条带伪影）
    float jitter = 0.0;
    if (enableJittering) {
        jitter = random(TexCoord + time) * stepSize;
    }
    
    // 累积颜色和透明度
    vec4 accumulatedColor = vec4(0.0);
    vec3 currentPos = startPos + rayDir * jitter;
    float traveled = jitter;
    
    // Ray Marching主循环
    int steps = 0;
    while (traveled < rayLength && steps < maxSteps && accumulatedColor.a < 0.95) {
        // 将位置转换到纹理坐标空间 [0,1]
        vec3 texCoord = currentPos - boxMin;
        texCoord /= (boxMax - boxMin);
        
        // 采样体数据
        float densityValue = texture(volumeTexture, texCoord).r;
        
        // 应用密度系数和阈值
        densityValue *= density;
        
        if (densityValue > threshold) {
            // 从传输函数获取颜色
            vec4 sampledColor = texture(transferFunction, densityValue);
            
            // 应用光照
            if (enableLighting && sampledColor.a > 0.01) {
                vec3 gradient = computeGradient(texCoord);
                if (length(gradient) > 0.01) {
                    vec3 normal = normalize(gradient);
                    vec3 viewDir = normalize(cameraPos - currentPos);
                    sampledColor.rgb = computeLighting(normal, viewDir, sampledColor.rgb);
                }
            }
            
            // 调整透明度（基于步长和吸收系数）
            sampledColor.a *= stepSize * absorptionCoeff * 100.0;
            sampledColor.a = clamp(sampledColor.a, 0.0, 1.0);
            
            // 预乘Alpha混合
            sampledColor.rgb *= sampledColor.a;
            
            // Front-to-back合成
            accumulatedColor += (1.0 - accumulatedColor.a) * sampledColor;
        }
        
        // 前进一步
        currentPos += rayDir * stepSize;
        traveled += stepSize;
        steps++;
    }
    
    // 背景混合
    vec3 backgroundColor = vec3(0.1, 0.1, 0.15);
    vec3 finalColor = accumulatedColor.rgb + (1.0 - accumulatedColor.a) * backgroundColor;
    
    FragColor = vec4(finalColor, 1.0);
}
