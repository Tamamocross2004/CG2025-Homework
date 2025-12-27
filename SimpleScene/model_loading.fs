#version 330 core
out vec4 FragColor;

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoords;

struct Light {
    vec3 position;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

struct PointLight {
    vec3 position;
    vec3 color;
    float intensity;
};

uniform vec3 viewPos;
uniform Light light;
uniform sampler2D texture_diffuse1;

// 台灯点光源和开关
uniform PointLight lampLight;
uniform bool lampOn;
// 标识是否是台灯模型
uniform bool isLampModel;

void main()
{    
    vec3 texColor = texture(texture_diffuse1, TexCoords).rgb;

    // ===== 主光源（房间顶灯） =====
    // ambient
    vec3 ambient = light.ambient * texColor;
      
    // diffuse 
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(light.position - FragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = light.diffuse * diff * texColor;
    
    // specular
    vec3 viewDir    = normalize(viewPos - FragPos);
    vec3 reflectDir = reflect(-lightDir, norm);  
    float spec      = pow(max(dot(viewDir, reflectDir), 0.0), 16.0); 
    vec3 specular   = light.specular * spec * texColor;
        
    vec3 result = ambient + diffuse + specular;

    // ===== 台灯点光源 =====
    if (lampOn) {
        vec3 L = normalize(lampLight.position - FragPos);
        float d = length(lampLight.position - FragPos);

        // 衰减
        float att = 1.0 / (1.0 + 0.35 * d + 0.44 * d * d);

        float diffL = max(dot(norm, L), 0.0);
        vec3 diffuseL = lampLight.color * diffL * texColor;

        vec3 R = reflect(-L, norm);
        float specL = pow(max(dot(viewDir, R), 0.0), 32.0);
        vec3 specularL = lampLight.color * specL * texColor * 0.6;

        result += att * (diffuseL + specularL) * lampLight.intensity;
    }   

    // ===== 台灯自发光（灯罩发光效果） =====
    vec3 emission = vec3(0.0);
    if(lampOn && isLampModel) {
        // 1. 基于法线方向：只有法线朝上的部分（灯罩顶部和侧面）发光
        // 台灯通常向上发光，所以法线向上的部分应该更亮
        float upFactor = max(dot(norm, vec3(0.0, 1.0, 0.0)), 0.0);
        float sideFactor = 1.0 - abs(dot(norm, vec3(0.0, 1.0, 0.0))); // 侧面因子
        float normalFactor = upFactor * 0.6 + sideFactor * 0.4; // 顶部更亮，侧面稍暗
        
        // 2. 基于视角的菲涅尔效果：从侧面看时边缘更亮（使用已定义的 viewDir）
        float fresnel = 1.0 - max(dot(viewDir, norm), 0.0);
        fresnel = pow(fresnel, 3.0); // 调整菲涅尔强度
        
        // 3. 基于距离光源的距离：靠近光源中心的部分更亮
        vec3 toLight = normalize(lampLight.position - FragPos);
        float distToLight = length(lampLight.position - FragPos);
        float distFactor = 1.0 - smoothstep(0.0, 1.5, distToLight); // 0.5米内最亮
        
        // 4. 基于纹理亮度：较亮的区域（可能是灯罩材质）发光更强
        float brightness = dot(texColor, vec3(0.299, 0.587, 0.114));
        float textureMask = smoothstep(0.2, 0.9, brightness);
        
        // 5. 综合所有因子
        float emissionIntensity = normalFactor * (0.6 + fresnel * 0.4) * distFactor * textureMask;
        
        // 6. 使用暖黄色发光，并与原始纹理颜色混合（而不是纯白色）
        vec3 emissionColor = mix(texColor, lampLight.color, 0.7); // 70% 暖黄 + 30% 原色
        emission = emissionColor * emissionIntensity * 4.0; // 整体强度，可调
        
        // 7. 添加一个柔和的边缘光效果
        float edgeGlow = smoothstep(0.3, 0.7, fresnel) * normalFactor;
        emission += lampLight.color * edgeGlow * 0.8;
        
        // 将自发光加到最终结果
        result += emission;
    }

    FragColor = vec4(result, 1.0);
} 