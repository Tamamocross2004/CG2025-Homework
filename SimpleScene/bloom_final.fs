#version 330 core
out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D sceneTexture;  // 原始场景
uniform sampler2D bloomTexture;  // 模糊后的Bloom纹理
uniform float bloomStrength;     // Bloom强度，例如 0.5

void main()
{
    vec3 hdrColor = texture(sceneTexture, TexCoords).rgb;
    vec3 bloomColor = texture(bloomTexture, TexCoords).rgb;
    
    // 将Bloom效果添加到原始场景（加法混合）
    vec3 result = hdrColor + bloomColor * bloomStrength;
    
    // 简单色调映射（防止过曝，可选）
    result = vec3(1.0) - exp(-result * 1.0);  // 可选的色调映射
    
    FragColor = vec4(result, 1.0);
}