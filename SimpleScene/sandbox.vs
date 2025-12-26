#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;
layout (location = 3) in vec3 aTangent;
layout (location = 4) in vec3 aBitangent;
layout (location = 5) in float aIsTopSurface;

out VS_OUT {
    vec2 TexCoords;
    vec3 TangentLightPos;
    vec3 TangentViewPos;
    vec3 TangentFragPos;
    float isTopSurface;
    vec3 TangentLampLightPos;
} vs_out;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

uniform vec3 lightPos;
uniform vec3 viewPos;

// 台灯光源位置(世界空间)
uniform vec3 lampLightPos;
uniform bool lampOn; 

void main()
{
    vec4 fragPosWorld = model * vec4(aPos, 1.0);
    vs_out.TexCoords = aTexCoords;
    vs_out.isTopSurface = aIsTopSurface;

    mat3 normalMatrix = transpose(inverse(mat3(model)));
    vec3 T = normalize(normalMatrix * aTangent);
    vec3 B = normalize(normalMatrix * aBitangent);
    vec3 N = normalize(normalMatrix * aNormal);
    mat3 TBN = transpose(mat3(T, B, N));

    vs_out.TangentLightPos = TBN * lightPos;
    vs_out.TangentViewPos  = TBN * viewPos;
    vs_out.TangentFragPos  = TBN * fragPosWorld.xyz;

    // 转换台灯光源到切线空间
    if (lampOn) {
        vs_out.TangentLampLightPos = TBN * lampLightPos;
    } else {
        vs_out.TangentLampLightPos = vec3(0.0);
    }

    gl_Position = projection * view * fragPosWorld;
}