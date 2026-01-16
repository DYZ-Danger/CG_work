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
uniform float alphaScale;
uniform float shadowMin;
uniform float shadowAtten;

// 摄像机
uniform mat4 invView;
uniform mat4 invProjection;
uniform vec3 cameraPos;
uniform float time;

//常量
const float PI = 3.14159265359;
const vec3 boxMin = vec3(-0.5);
const vec3 boxMax = vec3(0.5);
const float edgeFadeWidth = 0.08;

// 随机函数（保持原样）
float random(vec2 st) {
    return fract(sin(dot(st.xy, vec2(12.9898, 78.233))) * 43758.5453123);
}

// 随机3D（保持原样）
float random3(vec3 p) {
    return fract(sin(dot(p, vec3(12.9898, 78.233, 37.719))) * 43758.5453123);
}

float phaseHG(float cosTheta, float g) {
    float g2 = g * g;
    float denom = pow(1.0 + g2 - 2.0 * g * cosTheta, 1.5);
    return (1.0 - g2) / (4.0 * 3.14159265 * denom);
}

// AABB相交检测
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

float sampleDensity(vec3 texCoord) {
    return texture(volumeTexture, texCoord).r * density;
}

vec3 computeGradient(vec3 pos) {
    float offset = 0.004;
    vec3 p = clamp(pos, vec3(offset), vec3(1.0 - offset));
    float dx = texture(volumeTexture, p + vec3(offset, 0, 0)).r - texture(volumeTexture, p - vec3(offset, 0, 0)).r;
    float dy = texture(volumeTexture, p + vec3(0, offset, 0)).r - texture(volumeTexture, p - vec3(0, offset, 0)).r;
    float dz = texture(volumeTexture, p + vec3(0, 0, offset)).r - texture(volumeTexture, p - vec3(0, 0, offset)).r;
    return normalize(vec3(dx, dy, dz));
}

// 阴影计算
float computeShadow(vec3 worldPos, vec3 lightDirW, float sigmaT) {
    float stepWorld = stepSize * 4.5;
    float trans = 1.0;

    // 阴影抖动（保持原样）
    float sj = (random3(worldPos + time) - 0.5) * stepWorld * 0.6;
    worldPos += lightDirW * sj;

    for (int i = 0; i < 16; ++i) {
        worldPos += lightDirW * stepWorld;
        vec3 texCoord = (worldPos - boxMin) / (boxMax - boxMin);
        if (any(lessThan(texCoord, vec3(0.0))) || any(greaterThan(texCoord, vec3(1.0))))
            break;
        float d = sampleDensity(texCoord);
        if (d > 0.001) {
            float att = exp(-d * sigmaT * stepWorld * 45.0 * shadowAtten);
            trans *= att;
            if (trans < 0.20) break;
        }
    }
    return clamp(trans, 0.0, 1.0);
}

// 天空背景函数
vec3 atmosphereSky(vec3 viewDir, vec3 sunDir)
{
    // [修复点1] 这里的 viewDir 应该是视线方向(rayDir)，sunDir 应该是指向太阳的方向
    float mu = clamp(dot(viewDir, sunDir), -1.0, 1.0);
    
    // 增强散射系数，让天空更明显
    vec3 betaR = vec3(0.3, 0.5, 1.0);  // 蓝天色调，进一步增强
    float rayleighPhase = 3.0 / (16.0 * PI) * (1.0 + mu * mu);
    
    float g = 0.76;
    float miePhase = (1.0 - g * g) / (4.0 * PI * pow(1.0 + g * g - 2.0 * g * mu, 1.5));
    
    // 大幅提升强度，让效果可见
    float rayleighStrength = 2.0;   // 蓝天强度
    float mieStrength      = 1.0;   // 雾霾/大气光晕
    float sunIntensity     = 20.0;  // 太阳亮度

    vec3 sky = rayleighPhase * betaR * rayleighStrength +
               miePhase * vec3(0.9, 0.92, 0.95) * mieStrength;
    
    // 高度渐变：天顶亮、地平线暗
    float horizon = smoothstep(-0.2, 0.4, viewDir.y);
    sky *= (0.4 + 0.6 * horizon);  // 地平线保留40%亮度

    // 太阳光晕：更宽的高光+强度
    float sunDisk = pow(max(mu, 0.0), 256.0) * sunIntensity * 3.0;      // 太阳核心更锐利
    float sunGlow = pow(max(mu, 0.0), 6.0) * sunIntensity * 0.5;        // 太阳光晕更宽
    sky += vec3(1.0, 0.98, 0.9) * (sunDisk + sunGlow);
    
    return max(sky, vec3(0.1, 0.15, 0.25));  // 最暗处保留蓝色夜空
}

void main() {
    // 1. 重建光线
    vec4 clipPos = vec4(TexCoord * 2.0 - 1.0, -1.0, 1.0);
    vec4 viewPos = invProjection * clipPos;
    viewPos /= viewPos.w;
    
    vec3 rayDir = normalize((invView * vec4(viewPos.xyz, 0.0)).xyz);
    vec3 rayOrigin = cameraPos;
    
    // [保留原样] 用于光照计算的视线向量 (指向相机)
    vec3 viewDir  = normalize(-rayDir); 

    // [修复] lightDir 已经是"光线入射方向"（从太阳指向场景），所以 sunDir 应该取反
    // 如果你在 UI 里设置 lightDir 为 (0,1,0)，表示光从上往下照，那太阳应该在上方
    // 所以这里不取反，直接用 lightDir 的反方向作为"指向太阳的方向"
    vec3 sunDir = normalize(lightDir);  // 直接用 lightDir（光入射的反方向=指向太阳）

    // 计算环境光颜色 (用于体积内照明)
    vec3 skyLight = atmosphereSky(rayDir, sunDir); 

    float tNear, tFar;
    // [修复点3] 如果未击中包围盒，直接渲染天空
    if (!intersectAABB(rayOrigin, rayDir, boxMin, boxMax, tNear, tFar)) {
        // 关键修复：这里传入 rayDir (视线)，而不是 viewDir (反向)
        vec3 skyColor = atmosphereSky(rayDir, sunDir);
        FragColor = vec4(skyColor, 1.0);
        return;
    }
    
    if (tNear < 0.0) tNear = 0.0;
    tNear += 1e-3;
    
    vec3 startPos = rayOrigin + rayDir * tNear;
    vec3 endPos = rayOrigin + rayDir * tFar;
    float rayLength = distance(startPos, endPos);
    
    // ==========================================
    // [重要] 抖动采样 (完全保留原逻辑)
    // ==========================================
    float jitter = 0.0;
    if (enableJittering) {
        vec3 seed = rayOrigin + rayDir * (tNear + 0.123);
        jitter = random3(seed + time) * stepSize;
    }
    
    vec4 accumulatedColor = vec4(0.0);
    vec3 currentPos = startPos + rayDir * jitter;
    float traveled = jitter;
    int steps = 0;

    while (traveled < rayLength && steps < maxSteps && accumulatedColor.a < 0.98) {
        vec3 texCoord = (currentPos - boxMin) / (boxMax - boxMin);

        if (any(lessThan(texCoord, vec3(0.0))) || any(greaterThan(texCoord, vec3(1.0)))) break;

        float densityValue = sampleDensity(texCoord);
        float gradMag = 0.0;

        if (densityValue > threshold) {
            // 边缘和高度衰减逻辑 (保持不变)
            float edgeDist = min(min(texCoord.x, 1.0 - texCoord.x), min(texCoord.y, 1.0 - texCoord.y));
            edgeDist = min(edgeDist, min(texCoord.z, 1.0 - texCoord.z));
            float edgeFade = smoothstep(0.0, edgeFadeWidth, edgeDist);

            float softDensity = smoothstep(threshold - 0.02, threshold + 0.05, densityValue);
            float height = (currentPos.y - boxMin.y) / (boxMax.y - boxMin.y);
            float heightFade = smoothstep(0.0, 0.2, height) * smoothstep(1.0, 0.7, height);
            softDensity *= heightFade;

            float sigmaT = absorptionCoeff * 0.9 + 0.10;
            float alpha = 1.0 - exp(-softDensity * sigmaT * stepSize * 42.0);
            alpha = clamp(alpha, 0.0, 1.0);
            alpha *= edgeFade * alphaScale;

            vec4 sampledColor = texture(transferFunction, densityValue);
            sampledColor.a = alpha;

            if (enableLighting && sampledColor.a > 0.01) {
                vec3 gradient = computeGradient(texCoord);
                float g = clamp(scatteringCoeff * 0.7, 0.0, 0.75);
                float phase = phaseHG(dot(lightDir, viewDir), g); // 这里 lightDir 不变，因HG需要入射光线
                float shadow = computeShadow(currentPos, normalize(lightDir), sigmaT);
                
                vec3 directLight = sampledColor.rgb;

                if (length(gradient) > 0.02) {
                    vec3 normal = normalize(gradient);
                    float diff = max(dot(normal, -normalize(lightDir)), 0.0); // 漫反射修正
                    float spec = pow(max(dot(viewDir, reflect(normalize(lightDir), normal)), 0.0), 16.0);
                    float lighting = 0.55 + diff * 0.65 + spec * 0.22;
                    float shadowTerm = mix(shadowMin, 1.0, shadow);
                    directLight *= lighting * shadowTerm * (0.5 + 0.5 * phase);
                } else {
                    float shadowTerm = mix(shadowMin, 1.0, shadow);
                    directLight *= shadowTerm * (0.5 + 0.5 * phase);
                }

                // 增强环境光影响，让云被天空照亮更明显
                vec3 ambientLight = skyLight * 0.6 * (1.0 - sampledColor.a * 0.5);
                sampledColor.rgb = directLight + ambientLight;
            }

            sampledColor.rgb *= sampledColor.a;
            accumulatedColor += (1.0 - accumulatedColor.a) * sampledColor;
        }

        // 计算梯度模长用于自适应步长
        if (enableLighting && densityValue > threshold && (steps & 1) == 0) {
            float o = 0.004;
            vec3 p = clamp(texCoord, vec3(o), vec3(1.0 - o));
            float dx = texture(volumeTexture, p + vec3(o,0,0)).r - texture(volumeTexture, p - vec3(o,0,0)).r;
            float dy = texture(volumeTexture, p + vec3(0,o,0)).r - texture(volumeTexture, p - vec3(0,o,0)).r;
            float dz = texture(volumeTexture, p + vec3(0,0,o)).r - texture(volumeTexture, p - vec3(0,0,o)).r;
            gradMag = abs(dx) + abs(dy) + abs(dz);
        }

        // ==========================================
        // [重要] 自适应步长 (完全保留原逻辑)
        // ==========================================
        float densityFactor = clamp(densityValue, 0.0, 1.0);
        float gradFactor    = clamp(gradMag * 6.0, 0.0, 1.0);
        float opacityFactor = 1.0 - accumulatedColor.a;
        
        float adaptiveStep = stepSize;
        adaptiveStep *= mix(1.8, 0.4, densityFactor);   
        adaptiveStep *= mix(1.6, 0.6, gradFactor);      
        adaptiveStep *= mix(1.6, 0.5, 1.0 - opacityFactor);
        
        float stepJ = enableJittering ? (random3(currentPos + time) - 0.5) * stepSize * 0.6 : 0.0;
        float marchStep = clamp(adaptiveStep + stepJ, stepSize * 0.25, stepSize * 2.0);
        
        currentPos += rayDir * marchStep;
        traveled += marchStep;
        steps++;
    }
    
    // [修复点4] 背景混合：这里同样要使用 correct 的 rayDir 和 sunDir
    vec3 skyColor = atmosphereSky(rayDir, sunDir);
    vec3 finalColor = accumulatedColor.rgb + (1.0 - accumulatedColor.a) * skyColor;
    
    FragColor = vec4(finalColor, 1.0);
}