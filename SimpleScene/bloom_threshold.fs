#version 330 core
out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D sceneTexture;
uniform float threshold;  // 亮度阈值，例如 0.8

void main()
{
    vec3 hdrColor = texture(sceneTexture, TexCoords).rgb;
    
    // 计算亮度（使用标准灰度公式）
    float brightness = dot(hdrColor, vec3(0.2126, 0.7152, 0.0722));
    
    // 提取超过阈值的区域
    vec3 result = hdrColor;
    if (brightness < threshold) {
        result = vec3(0.0);  // 低于阈值，设为黑色
    }
    
    FragColor = vec4(result, 1.0);
}