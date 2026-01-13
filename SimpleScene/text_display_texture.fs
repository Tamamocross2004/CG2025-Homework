#version 330 core
out vec4 FragColor;

uniform sampler2D textTexture;
uniform float alpha;
uniform vec2 screenSize;

in vec2 fragPos;  // 片段位置（屏幕像素坐标）
in vec2 texCoords; // 纹理坐标

void main()
{
    // 翻转纹理坐标的Y轴（因为屏幕坐标Y轴被翻转了，需要对应翻转纹理坐标）
    vec2 flippedTexCoords = vec2(texCoords.x, 1.0 - texCoords.y);
    
    // 采样纹理
    vec4 texColor = texture(textTexture, flippedTexCoords);
    
    // 应用透明度
    FragColor = vec4(texColor.rgb, texColor.a * alpha);
}

