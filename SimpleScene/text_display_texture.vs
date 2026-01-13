#version 330 core
layout (location = 0) in vec2 aPos;
layout (location = 1) in vec2 aTexCoord;

uniform vec2 screenSize;

out vec2 fragPos;
out vec2 texCoords;

void main()
{
    // 将屏幕坐标转换为NDC坐标
    vec2 ndc = (aPos / screenSize) * 2.0 - 1.0;
    ndc.y = -ndc.y; // 翻转Y轴
    gl_Position = vec4(ndc, 0.0, 1.0);
    
    // 传递片段位置（屏幕像素坐标）和纹理坐标
    fragPos = aPos;
    texCoords = aTexCoord;
}

