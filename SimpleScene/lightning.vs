#version 330 core
layout (location = 0) in vec3 aPos; 
layout (location = 1) in vec2 aTexCoords;

out vec2 TexCoords;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform vec3 cameraPos;

void main()
{
    TexCoords = aTexCoords;

    vec3 center_world = vec3(model[3]);

    // 先在 xz 平面计算一个 2D 的方向向量
    vec2 look_xz = normalize(cameraPos.xz - center_world.xz);
    // 然后用这个 2D 向量构建一个 3D 向量，并将 y 分量设为 0
    vec3 look = vec3(look_xz.x, 0.0, look_xz.y);

    // 计算右向量
    vec3 right = vec3(look.z, 0.0, -look.x);
    vec3 up = vec3(0.0, 1.0, 0.0);

    // 获取闪电的缩放（高度和宽度）
    float width = length(model[0]);
    float height = length(model[1]);

    // 计算顶点在世界空间中的最终位置
    vec3 vertexPosition_worldspace = center_world
                                   + right * aPos.x * width
                                   + up * aPos.y * height;

    gl_Position = projection * view * vec4(vertexPosition_worldspace, 1.0);
}