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

    // 根据世界坐标获取地形高度
    float getHeight(float worldX, float worldZ);

    // 获取地形尺寸
    float getTerrainWidth() const { return terrainWidth; }
    float getTerrainDepth() const { return terrainDepth; }

private:
    unsigned int VAO, VBO, EBO;
    std::vector<float> vertices;
    std::vector<unsigned int> indices;
    unsigned int diffuseTexture;
    unsigned int normalTexture;
    int indexCount;
    float baseHeight;

    // 存储地形尺寸和顶点数据以供查询
    float terrainWidth;
    float terrainDepth;
    int terrainResolution;
    std::vector<glm::vec3> terrainPositions;

    // 生成顶点数据
    void generateMesh(float width, float depth, int resolution, GenMethod method, const char* heightmapPath);
    // 设置OpenGL缓冲
    void setupMesh();
    // 加载纹理
    unsigned int loadTexture(const char* path);
};
