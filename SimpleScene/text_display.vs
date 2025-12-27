#version 330 core
layout (location = 0) in vec2 aPos;

uniform vec2 screenSize;

void main()
{
    // 将屏幕坐标转换为NDC坐标
    vec2 ndc = (aPos / screenSize) * 2.0 - 1.0;
    ndc.y = -ndc.y; // 翻转Y轴
    gl_Position = vec4(ndc, 0.0, 1.0);
}

