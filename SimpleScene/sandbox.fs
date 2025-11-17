#version 330 core
out vec4 FragColor;

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoords;

uniform vec3 viewPos;
uniform vec3 lightPos;
uniform vec3 lightColor;
uniform sampler2D texture_diffuse1;
uniform sampler2D texture_normal1;

// 一个简单的函数，从灰度图创建扰动法线
vec3 getNormalFromMap()
{
    // 从法线贴图（灰度图）中采样
    float height = texture(texture_normal1, TexCoords).r;

    // 计算相邻像素的UV坐标
    vec2 texelSize = 1.0 / textureSize(texture_normal1, 0);
    float hx = texture(texture_normal1, TexCoords + vec2(texelSize.x, 0.0)).r;
    float hy = texture(texture_normal1, TexCoords + vec2(0.0, texelSize.y)).r;

    // 计算梯度，创建一个扰动向量
    vec3 tangentNormal = vec3(height - hx, height - hy, 0.1); // 0.1 控制凹凸强度

    // 将扰动法线从切线空间转换到世界空间
    vec3 T = normalize(dFdx(FragPos));
    vec3 B = normalize(dFdy(FragPos));
    vec3 N = normalize(Normal);
    mat3 TBN = mat3(T, B, N);

    return normalize(TBN * tangentNormal);
}

void main()
{
    // 从法线贴图获取法线，而不是直接使用模型的法线
    vec3 norm = getNormalFromMap();
    
    // 环境光
    float ambientStrength = 0.2;
    vec3 ambient = ambientStrength * lightColor;

    // 漫反射
    vec3 lightDir = normalize(lightPos - FragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * lightColor;

    // 镜面反射
    float specularStrength = 0.3;
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 reflectDir = reflect(-lightDir, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32);
    vec3 specular = specularStrength * spec * lightColor;

    vec3 result = (ambient + diffuse + specular) * texture(texture_diffuse1, TexCoords).rgb;
    FragColor = vec4(result, 1.0);
}