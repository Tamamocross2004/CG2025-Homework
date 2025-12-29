#version 330 core
out vec4 FragColor;

in vec3 FragPos;
in vec3 Normal;

uniform vec3 viewPos;
uniform float time; // 时间，用于颜色变化

// 主光源
uniform vec3 lightPos;
uniform vec3 lightColor;

// 台灯点光源
struct PointLight {
    vec3 position;
    vec3 color;
    float intensity;
};

uniform PointLight lampLight;
uniform bool lampOn;

void main()
{
    // 计算随时间变化的颜色（使用正弦波实现平滑的颜色循环）
    vec3 baseColor;
    baseColor.r = 0.5 + 0.5 * sin(time * 2.0 + 0.0);        // 红色分量
    baseColor.g = 0.5 + 0.5 * sin(time * 2.0 + 2.094);      // 绿色分量（相位偏移120度）
    baseColor.b = 0.5 + 0.5 * sin(time * 2.0 + 4.189);      // 蓝色分量（相位偏移240度）
    
    // 归一化颜色，使其更鲜艳
    baseColor = normalize(baseColor) * 0.8 + baseColor * 0.2;
    
    // 添加自发光效果，使灵珠更亮
    vec3 emission = baseColor * 2.0;
    
    // 环境光
    float ambientStrength = 0.3;
    vec3 ambient = ambientStrength * lightColor;
    
    // 漫反射
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(lightPos - FragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * lightColor;
    
    // 镜面反射（高光）
    float specularStrength = 0.8;
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 reflectDir = reflect(-lightDir, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 64.0);
    vec3 specular = specularStrength * spec * lightColor;
    
    // 主光源结果
    vec3 result = (ambient + diffuse + specular) * baseColor + emission;
    
    // 台灯点光源
    if (lampOn) {
        vec3 L = normalize(lampLight.position - FragPos);
        float d = length(lampLight.position - FragPos);
        
        // 衰减
        float att = 1.0 / (1.0 + 0.35 * d + 0.44 * d * d);
        
        float diffL = max(dot(norm, L), 0.0);
        vec3 diffuseL = lampLight.color * diffL * baseColor;
        
        vec3 R = reflect(-L, norm);
        float specL = pow(max(dot(viewDir, R), 0.0), 64.0);
        vec3 specularL = lampLight.color * specL * baseColor * 0.8;
        
        result += att * (diffuseL + specularL) * lampLight.intensity;
    }
    
    FragColor = vec4(result, 1.0);
}

