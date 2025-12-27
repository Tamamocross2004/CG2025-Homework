#version 330 core
layout (location = 0) in vec3 aPos;

uniform mat4 projection;
uniform mat4 view;
uniform vec3 centerPos; // Billboard中心位置
uniform vec2 size;      // Billboard大小

out vec2 TexCoord;

void main()
{
    // 计算相机右向量和上向量（从view矩阵的逆矩阵提取）
    // view矩阵的逆矩阵的前3x3部分是相机空间的基向量
    mat3 viewRot = mat3(view);
    mat3 invViewRot = transpose(viewRot); // 对于旋转矩阵，转置就是逆
    
    vec3 right = invViewRot[0]; // 右向量
    vec3 up = invViewRot[1];    // 上向量
    
    // 计算billboard的四个角
    vec3 pos = centerPos;
    pos += right * (aPos.x * size.x);
    pos += up * (aPos.y * size.y);
    
    gl_Position = projection * view * vec4(pos, 1.0);
    
    // 纹理坐标（基于aPos的x和y）
    TexCoord = vec2(aPos.x + 0.5, aPos.y + 0.5);
}

