#version 330 core
out vec4 FragColor;

uniform vec2 center;
uniform float radius;
uniform float scale;

void main()
{
    float dist = distance(gl_FragCoord.xy, center * scale);
    float intensity = 1.0 - smoothstep(0.0, radius, dist);
    
    // 只在红色通道上绘制，作为蒙版值
    FragColor = vec4(intensity, 0.0, 0.0, 1.0);
}