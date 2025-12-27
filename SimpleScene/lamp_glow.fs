#version 330 core
out vec4 FragColor;

in vec2 TexCoords;

uniform vec3 glowColor;
uniform float glowIntensity;

void main()
{
    // 计算从中心到边缘的距离
    vec2 center = vec2(0.5, 0.5);
    float dist = length(TexCoords - center);

    // 使用smoothstep函数来创建柔和的渐变效果
    float alpha = 1.0 - smoothstep(0.0, 0.7, dist);

    // 添加一些径向渐变效果
    float radialGradient = 1.0 - dist * 1.4;
    radialGradient = max(radialGradient, 0.0);

    // 综合所有效果
    vec3 finalColor = glowColor * glowIntensity;
    FragColor = vec4(finalColor, alpha * radialGradient);
}