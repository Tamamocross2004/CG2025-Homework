#version 330 core
out vec4 FragColor;

in VS_OUT {
    vec2 TexCoords;
    vec3 TangentLightPos;
    vec3 TangentViewPos;
    vec3 TangentFragPos;
    float isTopSurface;
    vec3 TangentLampLightPos;
} fs_in;

uniform vec3 lightColor;
// 台灯点光源
struct PointLight {
    vec3 color;
    float intensity;
};

uniform PointLight lampLight;
uniform bool lampOn;

uniform sampler2D texture_diffuse1;
uniform sampler2D texture_grass1;
uniform sampler2D texture_snow1;
uniform sampler2D texture_effect_mask;
uniform sampler2D texture_normal1;

void main()
{    
    // 光照计算所需的通用变量
    vec3 norm = normalize(texture(texture_normal1, fs_in.TexCoords).rgb * 2.0 - 1.0);
    float ambientStrength = 0.6;
    vec3 ambient = ambientStrength * lightColor;
    vec3 lightDir = normalize(fs_in.TangentLightPos - fs_in.TangentFragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * lightColor;
    float specularStrength = 0.3;
    vec3 viewDir = normalize(fs_in.TangentViewPos - fs_in.TangentFragPos);
    vec3 reflectDir = reflect(-lightDir, norm);  
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32);
    vec3 specular = specularStrength * spec * lightColor;
    vec3 lighting = ambient + diffuse + specular;

    // 台灯点光源(切线空间)
    if (lampOn) {
        vec3 L = normalize(fs_in.TangentLampLightPos - fs_in.TangentFragPos);
        float d = length(fs_in.TangentLampLightPos - fs_in.TangentFragPos);
        
        // 衰减
        float att = 1.0 / (1.0 + 0.35 * d + 0.44 * d * d);
        
        float diffL = max(dot(norm, L), 0.0);
        vec3 diffuseL = lampLight.color * diffL;
        
        vec3 R = reflect(-L, norm);
        float specL = pow(max(dot(viewDir, R), 0.0), 32.0);
        vec3 specularL = lampLight.color * specL * 0.5;
        
        lighting += (diffuseL + specularL) * lampLight.intensity * att;
    }


    // 使用标签来决定渲染路径
    if (fs_in.isTopSurface < 0.5) // 如果是侧壁或底部
    {
        vec3 sandColor = texture(texture_diffuse1, fs_in.TexCoords).rgb;
        FragColor = vec4(sandColor * lighting, 1.0);
        return; // 直接渲染沙子材质并退出
    }
    
    // 以下代码只对顶部表面执行
    vec4 sandColor = texture(texture_diffuse1, fs_in.TexCoords);
    vec4 grassColor = texture(texture_grass1, fs_in.TexCoords);
    vec4 snowColor = texture(texture_snow1, fs_in.TexCoords);
    vec4 effectMask = texture(texture_effect_mask, fs_in.TexCoords);

    float grassAmount = effectMask.r;
    float snowAmount = effectMask.g;

    vec4 baseColor = mix(sandColor, grassColor, grassAmount);
    vec4 finalObjectColor = mix(baseColor, snowColor, snowAmount);
        
    vec3 litColor = lighting * finalObjectColor.rgb;
    
    vec3 finalResult = mix(litColor, snowColor.rgb, snowAmount * 0.4);

    FragColor = vec4(finalResult, finalObjectColor.a);
}