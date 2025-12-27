#version 330 core
out vec4 FragColor;

in vec2 TexCoord;

uniform vec3 textColor;

void main()
{
    // 简单的E字绘制（使用纹理坐标）
    // 竖线
    if (TexCoord.x < 0.25 && TexCoord.y > 0.0 && TexCoord.y < 1.0) {
        FragColor = vec4(textColor, 1.0);
        return;
    }
    // 上横线
    if (TexCoord.y > 0.75 && TexCoord.x < 1.0) {
        FragColor = vec4(textColor, 1.0);
        return;
    }
    // 中横线
    if (TexCoord.y > 0.4 && TexCoord.y < 0.6 && TexCoord.x < 0.7) {
        FragColor = vec4(textColor, 1.0);
        return;
    }
    // 下横线
    if (TexCoord.y < 0.25 && TexCoord.x < 1.0) {
        FragColor = vec4(textColor, 1.0);
        return;
    }
    
    // 其他区域透明
    FragColor = vec4(0.0, 0.0, 0.0, 0.0);
}

