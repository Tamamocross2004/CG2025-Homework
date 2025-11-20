#version 330 core
out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D lightningTexture;

void main()
{
    vec4 texColor = texture(lightningTexture, TexCoords);
    // 如果像素几乎透明， 就丢弃它
    if(texColor.a < 0.1)\
        discard;\

    // 闪电本身不需要复杂光照，直接使用纹理颜色，并乘以一个高亮度值
    FragColor = vec4(texColor.rgb * 2.0, texColor.a);
}