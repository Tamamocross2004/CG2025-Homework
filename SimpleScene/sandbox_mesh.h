#pragma once

#include <vector>
#include <string>
#include <glad/glad.h>
#include <glm/glm.hpp>
#include "shader.h"
// #include "stb_image.h"
#include <iostream>

class TerrainSandbox {
public:
    // 地形生成方法枚举
    enum class GenMethod {
        HEIGHTMAP,
        PROCEDURAL_RANDOM
    };

    // 构造函数
    TerrainSandbox(float width, float depth, int resolution, GenMethod method, const char* heightmapPath = nullptr, const char* diffusePath = nullptr, const char* normalPath = nullptr);
    // 析构函数
    ~TerrainSandbox();

    // 绘制沙盘
    void Draw(Shader& shader);

private:
    unsigned int VAO, VBO, EBO;
    std::vector<float> vertices;
    std::vector<unsigned int> indices;
    unsigned int diffuseTexture;
    unsigned int normalTexture;
    int indexCount;
    float baseHeight;

    // 生成顶点数据
    void generateMesh(float width, float depth, int resolution, GenMethod method, const char* heightmapPath);
    // 设置OpenGL缓冲
    void setupMesh();
    // 加载纹理
    unsigned int loadTexture(const char* path);
};
