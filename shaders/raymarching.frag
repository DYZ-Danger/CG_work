#version 330 core
in vec2 TexCoord;
out vec4 FragColor;
// 纹理
uniform sampler3D volumeTexture;
uniform sampler1D transferFunction;
uniform sampler2D blueNoiseTexture;  // 新增：蓝色噪声纹理（例如64x64单通道）
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
// MSAA 参数
uniform int msaaSamplesUniform;
uniform float msaaRadiusUniform;
// 多重散射
uniform bool enableMultipleScattering;
uniform int multiScatterSteps;
uniform float multiScatterStrength;
// 通透度联动控制
uniform float alphaScale;
uniform float shadowMin;
uniform float shadowAtten;
// 摄像机
uniform mat4 invView;
uniform mat4 invProjection;
uniform vec3 cameraPos;
uniform float time;
// 体积尺寸
uniform vec3 volumeSize;
//常量
const float PI = 3.14159265359;
const float edgeFadeWidth = 0.08;
// 计算包围盒（基于体积实际尺寸）
vec3 getBoxMin() {
    vec3 normalizedSize = volumeSize / max(max(volumeSize.x, volumeSize.y), volumeSize.z);
    return -normalizedSize * 0.5;
}
vec3 getBoxMax() {
    vec3 normalizedSize = volumeSize / max(max(volumeSize.x, volumeSize.y), volumeSize.z);
    return normalizedSize * 0.5;
}
// 随机函数（保持原样）
float random(vec2 st) {
    return fract(sin(dot(st.xy, vec2(12.9898, 78.233))) * 43758.5453123);
}
// 改进的3D哈希函数（可选保留，但现在用纹理替换概率部分）
vec3 hash33(vec3 p3) {
    p3 = fract(p3 * vec3(0.1031, 0.1030, 0.0973));
    p3 += dot(p3, p3.yxz + 33.33);
    return fract((p3.xxy + p3.yxx) * p3.zyx);
}
float hash13(vec3 p3) {
    return hash33(p3).x;
}
float phaseHG(float cosTheta, float g) {
    float g2 = g * g;
    float denom = pow(1.0 + g2 - 2.0 * g * cosTheta, 1.5);
    return (1.0 - g2) / (4.0 * 3.14159265 * denom);
}
// AABB相交检测
bool intersectAABB(vec3 rayOrigin, vec3 rayDir, vec3 boxMin, vec3 boxMax, out float tNear, out float tFar) {
    tNear = -1e10;
    tFar = 1e10;
   
    for (int i = 0; i < 3; ++i) {
        float invD = 1.0 / rayDir[i];
        float t0 = (boxMin[i] - rayOrigin[i]) * invD;
        float t1 = (boxMax[i] - rayOrigin[i]) * invD;
       
        if (invD < 0.0) {
            float temp = t0;
            t0 = t1;
            t1 = temp;
        }
       
        tNear = max(tNear, t0);
        tFar = min(tFar, t1);
       
        if (tFar < tNear) return false;
    }
   
    return tFar > 0.0;
}
float sampleDensity(vec3 texCoord) {
    return texture(volumeTexture, texCoord).r * density;
}
vec3 computeGradient(vec3 pos) {
    float offset = 0.004;
    // 严格限制范围
    vec3 p = clamp(pos, vec3(offset * 2.0), vec3(1.0 - offset * 2.0));
   
    float dx = texture(volumeTexture, p + vec3(offset, 0, 0)).r - texture(volumeTexture, p - vec3(offset, 0, 0)).r;
    float dy = texture(volumeTexture, p + vec3(0, offset, 0)).r - texture(volumeTexture, p - vec3(0, offset, 0)).r;
    float dz = texture(volumeTexture, p + vec3(0, 0, offset)).r - texture(volumeTexture, p - vec3(0, 0, offset)).r;
   
    // 添加长度检查
    vec3 grad = vec3(dx, dy, dz);
    return length(grad) > 0.001 ? normalize(grad) : vec3(0.0);
}
// 阴影计算
float computeShadow(vec3 worldPos, vec3 lightDirW, float sigmaT) {
    vec3 boxMin = getBoxMin();
    vec3 boxMax = getBoxMax();
    float stepWorld = stepSize * 4.5;
    float trans = 1.0;
    // 阴影抖动
    vec2 noiseCoord = (gl_FragCoord.xy + vec2(time * 0.1)) / vec2(64.0);  // 假设64x64纹理，添加时间偏移避免静态
    float sj = (texture(blueNoiseTexture, noiseCoord).r - 0.5) * stepWorld * 0.6;
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
    float mu = clamp(dot(viewDir, sunDir), -1.0, 1.0);
    vec3 betaR = vec3(0.45, 0.75, 1.25);
    float rayleighPhase = 3.0 / (16.0 * PI) * (1.0 + mu * mu);
    float g = 0.76;
    float miePhase = (1.0 - g * g) / (4.0 * PI * pow(1.0 + g * g - 2.0 * g * mu, 1.5));
    float rayleighStrength = 3.2;
    float mieStrength = 1.0;
    float sunIntensity = 2.8;
    vec3 sky = rayleighPhase * betaR * rayleighStrength +
               miePhase * vec3(0.9, 0.92, 0.95) * mieStrength;
    float horizon = smoothstep(-0.2, 0.4, viewDir.y);
    sky *= (0.55 + 0.45 * horizon);
    //float sunDisk = pow(max(mu, 0.0), 256.0) * sunIntensity * 0.18;
    //float sunGlow = pow(max(mu, 0.0), 2.5) * sunIntensity * 0.22;
    //sky += vec3(1.0, 0.98, 0.9) * (sunDisk + sunGlow);
    return max(sky, vec3(0.18, 0.28, 0.38));
}
// 地面渲染函数：简单的水平面y=-1.5，带体积阴影、软阴影和天光反射
vec3 renderGround(vec3 rayOrigin, vec3 rayDir, vec3 sunDir) {
    vec3 boxMin = getBoxMin();
    vec3 boxMax = getBoxMax();
    float groundY = -1.5;
    if (rayDir.y >= -1e-4) return vec3(0.0);
    float t = (groundY - rayOrigin.y) / rayDir.y;
    if (t < 0.0) return vec3(0.0);
    vec3 hitPos = rayOrigin + rayDir * t;
    float checker = step(0.5, mod(floor(hitPos.x * 4.0) + floor(hitPos.z * 4.0), 2.0));
    vec3 baseColor = mix(vec3(0.85, 0.85, 0.85), vec3(0.65, 0.7, 0.8), checker);
    // 天空反射
    vec3 sky = atmosphereSky(normalize(reflect(rayDir, vec3(0,1,0))), sunDir);
    // 体积阴影：从地面点沿sunDir向上采样体积密度
    float shadow = 1.0;
    vec3 shadowPos = hitPos + sunDir * 0.01;
    for (int i = 0; i < 32; ++i) {
        vec3 texCoord = (shadowPos - boxMin) / (boxMax - boxMin);
        if (any(lessThan(texCoord, vec3(0.0))) || any(greaterThan(texCoord, vec3(1.0)))) break;
        float d = sampleDensity(texCoord);
        shadow *= exp(-d * 7.0); // 体积阴影衰减
        if (shadow < 0.15) break;
        shadowPos += sunDir * 0.045;
    }
    // 太阳软阴影
    float sunShadow = clamp(dot(sunDir, vec3(0,1,0)), 0.0, 1.0);
    float softShadow = mix(0.7, 1.0, sunShadow);
    float totalShadow = 1.0;
    // 混合天空反射和地面色
    return baseColor * totalShadow * 0.7 + sky * 0.3;
}
void main() {
    // 获取动态包围盒
    vec3 boxMin = getBoxMin();
    vec3 boxMax = getBoxMax();
   
    // 1. 重建光线
    vec4 clipPos = vec4(TexCoord * 2.0 - 1.0, -1.0, 1.0);
    vec4 viewPos = invProjection * clipPos;
    viewPos /= viewPos.w;
   
    vec3 rayDir = normalize((invView * vec4(viewPos.xyz, 0.0)).xyz);
    vec3 rayOrigin = cameraPos;
   
    // 用于光照计算的视线向量 (指向相机)
    vec3 viewDir = normalize(-rayDir);
    vec3 sunDir = -normalize(lightDir); // 指向太阳（假设lightDir是入射方向的反向）
    // 计算环境光颜色 (用于体积内照明)
    vec3 skyLight = atmosphereSky(rayDir, sunDir);
    float tNear, tFar;
    // 如果未击中包围盒，直接渲染天空
    if (!intersectAABB(rayOrigin, rayDir, boxMin, boxMax, tNear, tFar)) {
        // 地面渲染：如果视线朝下，优先渲染地面，否则渲染天空
        if (rayDir.y < -0.01) {
            vec3 groundColor = renderGround(rayOrigin, rayDir, sunDir);
            FragColor = vec4(groundColor, 1.0);
        } else {
            vec3 skyColor = atmosphereSky(rayDir, sunDir);
            FragColor = vec4(skyColor, 1.0);
        }
        return;
    }
   
    if (tNear < 0.0) tNear = 0.0;
    tNear += 1e-3;
   
    vec3 startPos = rayOrigin + rayDir * tNear;
    vec3 endPos = rayOrigin + rayDir * tFar;
    float rayLength = distance(startPos, endPos);
   
    // ==========================================
    // 抖动采样
    // ==========================================
    float jitter = 0.0;
    if (enableJittering) {
        // 使用蓝色噪声纹理采样抖动
        vec2 noiseCoord = gl_FragCoord.xy / vec2(64.0);  // 假设64x64纹理
        jitter = texture(blueNoiseTexture, noiseCoord).r * stepSize;
    }
   
    vec4 accumulatedColor = vec4(0.0);
    vec3 currentPos = startPos + rayDir * jitter;
    float traveled = jitter;
    int steps = 0;
    //多重采样
    int msaaSamples = msaaSamplesUniform;
    vec4 msaaColor = vec4(0.0);
    float msaaRadius = msaaRadiusUniform * stepSize;
    // MSAA 偏移垂直于 ray
    vec3 up = abs(rayDir.y) > 0.99 ? vec3(1,0,0) : vec3(0,1,0);
    vec3 right = normalize(cross(rayDir, up));
    vec3 up2 = cross(right, rayDir);
    for (int s = 0; s < msaaSamples; ++s) {
        float angle = 6.2831853 * float(s) / max(float(msaaSamples), 1.0);
        float r = msaaSamples > 1 ? msaaRadius * sqrt(float(s) / float(msaaSamples-1)) : 0.0;
        vec3 offset = (right * cos(angle) + up2 * sin(angle)) * r;

        // 为每个MSAA采样重新计算交点，以避免偏移导致的起始点不准确
        vec3 msaaRayOrigin = rayOrigin + offset;
        float msaaTNear, msaaTFar;
        if (!intersectAABB(msaaRayOrigin, rayDir, boxMin, boxMax, msaaTNear, msaaTFar)) {
            // 如果偏移射线未击中包围盒，使用天空颜色作为该采样的贡献
            vec3 skyColor = atmosphereSky(rayDir, sunDir);
            msaaColor += vec4(skyColor, 1.0);
            continue;
        }
        if (msaaTNear < 0.0) msaaTNear = 0.0;
        msaaTNear += 1e-3;

        // 为每个采样计算独特的抖动，以减少相关噪点
        vec2 msaaNoiseCoord = (gl_FragCoord.xy + vec2(float(s) * 17.0, float(s) * 23.0)) / vec2(64.0);
        float msaaJitter = enableJittering ? texture(blueNoiseTexture, fract(msaaNoiseCoord)).r * stepSize : 0.0;

        vec3 sampleStart = msaaRayOrigin + rayDir * (msaaTNear + msaaJitter);
        float sampleRayLength = msaaTFar - msaaTNear;
        vec3 samplePos = sampleStart;
        float sampleTraveled = msaaJitter;
        int sampleSteps = 0;
        vec4 sampleAccum = vec4(0.0);
        while (sampleTraveled < sampleRayLength && sampleSteps < maxSteps && sampleAccum.a < 0.98) {
            vec3 texCoord = (samplePos - boxMin) / (boxMax - boxMin);
            
            // 安全检查
            if (any(lessThan(texCoord, vec3(0.0))) || any(greaterThan(texCoord, vec3(1.0)))) break;
            
            float densityValue = sampleDensity(texCoord);
            float gradMag = 0.0;

            // =========================================================
            //  核心修复：边界保护与去相关化逻辑
            // =========================================================
            
            // 1. 计算点到最近表面的归一化距离
            float distToEdge = min(min(texCoord.x, 1.0 - texCoord.x), min(texCoord.y, 1.0 - texCoord.y));
            distToEdge = min(distToEdge, min(texCoord.z, 1.0 - texCoord.z));
            
            // 2. 增强型蓝噪声采样
            float stableTime = floor(time * 10.0) / 10.0;
            vec2 noiseCoord = (gl_FragCoord.xy + vec2(stableTime * 13.0, float(sampleSteps) * 1.7 + float(s) * 5.3)) / vec2(64.0);
            float blueRand = texture(blueNoiseTexture, fract(noiseCoord)).r;
            
            // 3. 计算跳过概率
            float prob = smoothstep(threshold - 0.05, threshold + 0.05, densityValue);
            
            // 4. 【关键】强制保护区逻辑
            // 规则：距离边界 0.06 以内的区域，或者刚开始走的前 8 步，强制 prob = 1.0 (不跳过)
            bool isProtected = (distToEdge < 0.06) || (sampleSteps < 8);
            float effectiveProb = isProtected ? 1.0 : prob;

            // 5. 执行优化跳过判断
            if (blueRand > effectiveProb) {
                float densityFactor = clamp(densityValue, 0.0, 1.0);
                float opacityFactor = 1.0 - sampleAccum.a;
                
                float adaptiveStep = stepSize;
                adaptiveStep *= mix(1.8, 0.5, densityFactor);
                adaptiveStep *= mix(1.6, 0.5, 1.0 - opacityFactor);
                
                // 边界处即使跳过，也要收紧步长
                float edgeLimit = smoothstep(0.0, 0.15, distToEdge);
                adaptiveStep = mix(stepSize * 0.8, adaptiveStep, edgeLimit);
                
                float marchStep = clamp(adaptiveStep, stepSize * 0.5, stepSize * 1.8);
                
                samplePos += rayDir * marchStep;
                sampleTraveled += marchStep;
                sampleSteps++;
                continue;
            }

            // =========================================================
            //  正常的采样与光照逻辑 (未被跳过)
            // =========================================================
            float soft = smoothstep(threshold - 0.03, threshold + 0.05, densityValue);
            float softDensity = densityValue * soft;
            
            // 高度渐变
            float height = (samplePos.y - boxMin.y) / (boxMax.y - boxMin.y);
            float topFade = exp(-pow(max(height - 0.85, 0.0) * 6.0, 2.0));
            softDensity *= topFade;
            
            float sigmaT = absorptionCoeff * 0.9 + 0.10;
            float alpha = 1.0 - exp(-softDensity * sigmaT * stepSize * 42.0);
            
            // 边缘淡出效果 (应用到 Alpha 上)
            float edgeFade = smoothstep(0.0, edgeFadeWidth, distToEdge);
            alpha = clamp(alpha, 0.0, 0.85) * edgeFade * alphaScale;
            
            vec4 sampledColor = texture(transferFunction, densityValue);
            sampledColor.a = alpha;
            
            if (enableLighting && sampledColor.a > 0.01) {
                vec3 gradient = computeGradient(texCoord);
                float g = clamp(scatteringCoeff * 0.7, 0.0, 0.75);
                float phase = phaseHG(dot(lightDir, viewDir), g);
                float shadow = computeShadow(samplePos, normalize(lightDir), sigmaT);
                
                vec3 directLight = vec3(1.0);
                if (length(gradient) > 0.02) {
                    vec3 normal = normalize(gradient);
                    float diff = max(dot(normal, -normalize(lightDir)), 0.0);
                    // 移除过高的 Specular 以减少高频噪点
                    float lighting = 0.55 + diff * 0.75;
                    float shadowTerm = mix(shadowMin, 1.0, shadow);
                    directLight *= lighting * shadowTerm * (0.5 + 0.5 * phase);
                } else {
                    float shadowTerm = mix(shadowMin, 1.0, shadow);
                    directLight *= shadowTerm * (0.5 + 0.5 * phase);
                }
                
                vec3 ambientLight = skyLight * 0.6 * (1.0 - sampledColor.a * 0.5);
                sampledColor.rgb = directLight + ambientLight;
            }

            // 近似多重散射（低频次、沿光向前）
            if (enableMultipleScattering && sampledColor.a > 0.01) {
                float extinction = absorptionCoeff + scatteringCoeff + 0.001;
                float albedo = scatteringCoeff / extinction;
                float g = clamp(scatteringCoeff * 0.7, 0.0, 0.75);  // 复用主循环中的g值
                float reducedSigma = sigmaT * (1.0 - g);
                
                float phase_ms = phaseHG(dot(lightDir, viewDir), g * 0.5);
    
                // 添加抖动以减少噪声（使用蓝色噪声纹理）
                vec2 msNoiseCoord = (gl_FragCoord.xy + vec2(time * 0.05, float(sampleSteps) * 0.3)) / vec2(64.0);
                float msJitter = texture(blueNoiseTexture, fract(msNoiseCoord)).r * stepSize * 1.5;
    
                vec3 msDir = normalize(lightDir);
                vec3 msPos = samplePos + msDir * (stepSize * 2.0 + msJitter);  // 调整起始偏移并添加抖动
                float opticalDepth = 0.0;
                float msStepBase = stepSize * 2.0;  // 基础步长稍减小以提高精度
                int effectiveSteps = min(multiScatterSteps, 8);  // 限制最大步数以优化性能
    
                for (int m = 0; m < effectiveSteps; ++m) {
                    vec3 msTc = (msPos - boxMin) / (boxMax - boxMin);
                    if (any(lessThan(msTc, vec3(0.0))) || any(greaterThan(msTc, vec3(1.0)))) break;
        
                    float dms = sampleDensity(msTc);
                    if (dms < 0.001) {  // 低密度区跳过计算
                        msPos += msDir * msStepBase;
                        continue;
                    }
        
                    // 自适应步长：基于密度调整步长（高密度区步长小）
                    float densityFactor = clamp(dms, 0.0, 1.0);
                    float msStep = msStepBase * mix(1.5, 0.8, densityFactor);
        
                    opticalDepth += dms * reducedSigma * msStep * 42.0;  // 使用与主循环一致的缩放常量，并应用reducedSigma
        
                    msPos += msDir * msStep;
        
                    if (opticalDepth > 10.0) break;  // 提前退出阈值
                }
    
                float reducedShadow = exp(-opticalDepth);
                float shadowTerm_ms = mix(shadowMin, 1.0, reducedShadow);
                float msTerm = multiScatterStrength * shadowTerm_ms * (0.5 + 0.5 * phase_ms);
                sampledColor.rgb += msTerm;
            }
            
            sampledColor.rgb *= sampledColor.a;
            sampleAccum += (1.0 - sampleAccum.a) * sampledColor;
            
            // 计算下一次迭代的自适应步长
            float densityFactor = clamp(densityValue, 0.0, 1.0);
            float opacityFactor = 1.0 - sampleAccum.a;
            float adaptiveStep = stepSize * mix(1.6, 0.6, densityFactor) * 
            mix(1.5, 0.7, 1.0 - opacityFactor);
            
            float marchStep = clamp(adaptiveStep, stepSize * 0.4, stepSize * 1.6);
            
            samplePos += rayDir * marchStep;
            sampleTraveled += marchStep;
            sampleSteps++;
        }
        // 如果体积不完全不透明，混合天空
        vec3 skyColor = atmosphereSky(rayDir, sunDir);
        sampleAccum.rgb += (1.0 - sampleAccum.a) * skyColor;
        msaaColor += sampleAccum;
    }
    msaaColor /= float(msaaSamples);
    accumulatedColor = msaaColor;
   
    FragColor = vec4(accumulatedColor.rgb, 1.0);
}