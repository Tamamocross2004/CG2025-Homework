#version 330 core
out vec4 FragColor;

in vec2 TexCoords;

uniform float glowIntensity;
uniform float time; // 时间，用于创建动画效果

void main()
{
    // 计算从中心到边缘的距离
    vec2 center = vec2(0.5, 0.5);
    float dist = length(TexCoords - center);
    
    // 创建柔和的径向渐变（从中心到边缘）
    float alpha = 1.0 - smoothstep(0.0, 0.7, dist);
    
    // 添加一些径向渐变效果
    float radialGradient = 1.0 - dist * 1.4;
    radialGradient = max(radialGradient, 0.0);
    
    // 创建动态颜色（基于时间，产生脉动效果）
    // 使用类似灵珠的颜色（紫蓝色系）
    vec3 baseColor = vec3(0.5, 0.7, 1.0); // 蓝色基础色
    float pulse = 0.5 + 0.5 * sin(time * 2.0); // 脉动效果
    vec3 glowColor = baseColor * (0.8 + 0.2 * pulse);
    
    // 综合颜色和渐变效果
    vec3 finalColor = glowColor * glowIntensity;
    FragColor = vec4(finalColor, alpha * radialGradient);
}

