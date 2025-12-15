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

// 通透度联动控制
uniform float alphaScale;   // 控制整体Alpha强度（越小越通透）
uniform float shadowMin;    // 控制阴影最暗值（越大越通透）
uniform float shadowAtten;  // 控制阴影衰减强度（越小越通透）

// 摄像机
uniform mat4 invView;
uniform mat4 invProjection;
uniform vec3 cameraPos;
uniform float time;

// 体积包围盒
const vec3 boxMin = vec3(-0.5);
const vec3 boxMax = vec3(0.5);
const float edgeFadeWidth = 0.08; // 边缘软衰减宽度，避免出现明显框线

// 随机函数（用于抖动采样）
float random(vec2 st) {
    return fract(sin(dot(st.xy, vec2(12.9898, 78.233))) * 43758.5453123);
}

// 基于三维位置的哈希随机，避免屏幕空间条纹同相位
float random3(vec3 p) {
    return fract(sin(dot(p, vec3(12.9898, 78.233, 37.719))) * 43758.5453123);
}

float phaseHG(float cosTheta, float g) {
    float g2 = g * g;
    float denom = pow(1.0 + g2 - 2.0 * g * cosTheta, 1.5);
    return (1.0 - g2) / (4.0 * 3.14159265 * denom);
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

// 采样密度（归一化纹理坐标）
float sampleDensity(vec3 texCoord) {
    return texture(volumeTexture, texCoord).r * density;
}

// 计算梯度（用于光照）
vec3 computeGradient(vec3 pos) {
    float offset = 0.004; // 较小偏移，避免过度平滑
    vec3 p = clamp(pos, vec3(offset), vec3(1.0 - offset));
    float dx = texture(volumeTexture, p + vec3(offset, 0, 0)).r - texture(volumeTexture, p - vec3(offset, 0, 0)).r;
    float dy = texture(volumeTexture, p + vec3(0, offset, 0)).r - texture(volumeTexture, p - vec3(0, offset, 0)).r;
    float dz = texture(volumeTexture, p + vec3(0, 0, offset)).r - texture(volumeTexture, p - vec3(0, 0, offset)).r;
    return normalize(vec3(dx, dy, dz));
}

float computeShadow(vec3 worldPos, vec3 lightDirW, float sigmaT) {
    // 低步数体积阴影估计
    float stepWorld = stepSize * 4.5; // 进一步加密阴影采样
    float trans = 1.0;

    // 为阴影射线添加一次性世界空间抖动，避免固定采样对齐
    float sj = (random3(worldPos + time) - 0.5) * stepWorld * 0.6;
    worldPos += lightDirW * sj;

    for (int i = 0; i < 16; ++i) {
        worldPos += lightDirW * stepWorld;
        vec3 texCoord = (worldPos - boxMin) / (boxMax - boxMin);
        if (any(lessThan(texCoord, vec3(0.0))) || any(greaterThan(texCoord, vec3(1.0))))
            break;
        float d = sampleDensity(texCoord);
        if (d > 0.001) {
            float att = exp(-d * sigmaT * stepWorld * 45.0 * shadowAtten); // UI控制阴影衰减
            trans *= att;
            if (trans < 0.20) break; // 进一步抬高阴影下限
        }
    }
    return clamp(trans, 0.0, 1.0);
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
    tNear += 1e-3; // 入箱处微偏移，避免交界暗线
    
    // Ray Marching初始化
    vec3 startPos = rayOrigin + rayDir * tNear;
    vec3 endPos = rayOrigin + rayDir * tFar;
    float rayLength = distance(startPos, endPos);
    
    // 抖动采样优化（减少条带伪影）
    float jitter = 0.0;
    if (enableJittering) {
        // 使用世界空间抖动，减少屏幕空间同频条纹
        vec3 seed = rayOrigin + rayDir * (tNear + 0.123);
        jitter = random3(seed + time) * stepSize;
    }
    
    // 累积颜色和透明度
    vec4 accumulatedColor = vec4(0.0);
    vec3 currentPos = startPos + rayDir * jitter;
    float traveled = jitter;
    
    // Ray Marching主循环
    int steps = 0;
    while (traveled < rayLength && steps < maxSteps && accumulatedColor.a < 0.98) {
        // 将位置转换到纹理坐标空间 [0,1]
        vec3 texCoord = (currentPos - boxMin) / (boxMax - boxMin);
        
        // 若超出范围，提前退出
        if (any(lessThan(texCoord, vec3(0.0))) || any(greaterThan(texCoord, vec3(1.0)))) break;
        
        // 采样体数据
        float densityValue = sampleDensity(texCoord);

        if (densityValue > threshold) {
            // 边缘软衰减，避免盒子边界留下硬边/暗框
            float edgeDist = min(min(texCoord.x, 1.0 - texCoord.x), min(texCoord.y, 1.0 - texCoord.y));
            edgeDist = min(edgeDist, min(texCoord.z, 1.0 - texCoord.z));
            float edgeFade = smoothstep(0.0, edgeFadeWidth, edgeDist);

            // 软阈值密度：放宽下限，让低密度也能被看见
            float softDensity = smoothstep(threshold - 0.02, threshold + 0.05, densityValue);

            // Beer-Lambert alpha，更自然的衰减
                float sigmaT = absorptionCoeff * 0.9 + 0.10; // 略降消光，缓和对比
                float alpha = 1.0 - exp(-softDensity * sigmaT * stepSize * 42.0);
            alpha = clamp(alpha, 0.0, 1.0);
            alpha *= edgeFade * alphaScale; // 由UI控制整体透明度

            // 从传输函数获取颜色
            vec4 sampledColor = texture(transferFunction, densityValue);
            sampledColor.a = alpha;

            // 光照（HG 相函数 + 体积阴影）
            if (enableLighting && sampledColor.a > 0.01) {
                vec3 gradient = computeGradient(texCoord);
                    float g = clamp(scatteringCoeff * 0.7, 0.0, 0.75); // 稍弱前向散射
                    float phase = phaseHG(dot(lightDir, normalize(cameraPos - currentPos)), g);
                    float shadow = computeShadow(currentPos, normalize(lightDir), sigmaT);

                // 如果梯度有效，做法线光照；否则仅相函数+阴影
                if (length(gradient) > 0.02) {
                    vec3 normal = normalize(gradient);
                    vec3 viewDir = normalize(cameraPos - currentPos);
                    float diff = max(dot(normal, lightDir), 0.0);
                    float spec = pow(max(dot(viewDir, reflect(-lightDir, normal)), 0.0), 16.0);
                        float lighting = 0.55 + diff * 0.65 + spec * 0.22; // 提高底光，减弱对比
                        float shadowTerm = mix(shadowMin, 1.0, shadow); // UI控制阴影下限
                        sampledColor.rgb *= lighting * shadowTerm * (0.5 + 0.5 * phase);
                } else {
                        float shadowTerm = mix(shadowMin, 1.0, shadow);
                        sampledColor.rgb *= shadowTerm * (0.5 + 0.5 * phase);
                }
            }

            // 预乘Alpha混合
            sampledColor.rgb *= sampledColor.a;
            
            // Front-to-back合成
            accumulatedColor += (1.0 - accumulatedColor.a) * sampledColor;
        }
        
        // 前进一步：密区更小步长，并叠加随机扰动抑制条带
        float adaptiveStep = mix(stepSize * 0.5, stepSize * 1.0, 1.0 - clamp(densityValue, 0.0, 1.0));
        float stepJ = enableJittering ? (random3(currentPos + time) - 0.5) * stepSize * 0.6 : 0.0;
        float marchStep = max(stepSize * 0.25, adaptiveStep + stepJ);
        currentPos += rayDir * marchStep;
        traveled += marchStep;
        steps++;
    }
    
    // 背景混合
    vec3 backgroundColor = vec3(0.22, 0.24, 0.30);
    vec3 finalColor = accumulatedColor.rgb + (1.0 - accumulatedColor.a) * backgroundColor;
    
    FragColor = vec4(finalColor, 1.0);
}
