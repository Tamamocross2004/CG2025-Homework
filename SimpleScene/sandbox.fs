#version 330 core
out vec4 FragColor;

in VS_OUT {
    vec3 FragPos;
    vec3 Normal;
    vec2 TexCoords;
} fs_in;

uniform vec3 viewPos;
uniform vec3 lightPos;
uniform vec3 lightColor;

uniform sampler2D texture_diffuse1;      // 沙子
uniform sampler2D texture_grass1;        // 草地
uniform sampler2D texture_snow1;         // 雪地
uniform sampler2D texture_effect_mask;   // 蒙版
uniform sampler2D texture_normal1;       // 法线贴图

void main()
{    
    // 从法线贴图获取法线
    vec3 norm = texture(texture_normal1, fs_in.TexCoords).rgb;
    // 将法线从 [0,1] 范围转换到 [-1,1] 范围，并使用模型传入的法线作为基础
    norm = normalize(norm * 2.0 - 1.0 + normalize(fs_in.Normal));

    // 采样所有纹理
    vec4 sandColor = texture(texture_diffuse1, fs_in.TexCoords);
    vec4 grassColor = texture(texture_grass1, fs_in.TexCoords);
    vec4 snowColor = texture(texture_snow1, fs_in.TexCoords);
    vec4 effectMask = texture(texture_effect_mask, fs_in.TexCoords);

    // 混合沙子和草地
    float grassAmount = effectMask.r; // 红色通道控制草
    vec4 baseColor = mix(sandColor, grassColor, grassAmount);

    // 在上一步结果的基础上，混合积雪
    float snowAmount = effectMask.g; // 绿色通道控制雪
    vec4 finalObjectColor = mix(baseColor, snowColor, snowAmount);

    // 环境光
    float ambientStrength = 0.6;
    vec3 ambient = ambientStrength * lightColor;
      
    // 漫反射 
    vec3 lightDir = normalize(lightPos - fs_in.FragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * lightColor;
    
    // 镜面反射
    float specularStrength = 0.3;
    vec3 viewDir = normalize(viewPos - fs_in.FragPos);
    vec3 reflectDir = reflect(-lightDir, norm);  
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32);
    vec3 specular = specularStrength * spec * lightColor;  
        
    vec3 result = (ambient + diffuse + specular) * finalObjectColor.rgb;
    FragColor = vec4(result, finalObjectColor.a);
}