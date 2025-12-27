#version 330 core
out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D image;
uniform bool horizontal;  // true = 水平模糊, false = 垂直模糊
uniform float blurSize;   // 模糊半径

// 5x5 高斯核的权重
const float weight[5] = float[] (0.227027, 0.1945946, 0.1216216, 0.054054, 0.016216);

void main()
{
    vec2 tex_offset = 1.0 / textureSize(image, 0); // 获取纹理的像素大小
    vec3 result = texture(image, TexCoords).rgb * weight[0]; // 中心采样
    
    if(horizontal)
    {
        for(int i = 1; i < 5; ++i)
        {
            result += texture(image, TexCoords + vec2(tex_offset.x * i * blurSize, 0.0)).rgb * weight[i];
            result += texture(image, TexCoords - vec2(tex_offset.x * i * blurSize, 0.0)).rgb * weight[i];
        }
    }
    else
    {
        for(int i = 1; i < 5; ++i)
        {
            result += texture(image, TexCoords + vec2(0.0, tex_offset.y * i * blurSize)).rgb * weight[i];
            result += texture(image, TexCoords - vec2(0.0, tex_offset.y * i * blurSize)).rgb * weight[i];
        }
    }
    
    FragColor = vec4(result, 1.0);
}