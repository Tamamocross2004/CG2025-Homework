#pragma once

#include <vector>
#include <string>
#include <glad/glad.h>
#include <glm/glm.hpp>
#include "shader.h"
// #include "stb_image.h"
#include <iostream>

struct TerrainVertex {
    glm::vec3 Position;
    glm::vec3 Normal;
    glm::vec2 TexCoords;
    glm::vec3 Tangent;     
    glm::vec3 Bitangent;   
    float isTopSurface;
};

class TerrainSandbox {
public:
    // 地形生成方法枚举
    enum class GenMethod {
        HEIGHTMAP,
        PROCEDURAL_RANDOM
    };

    // 构造函数
    TerrainSandbox(float width, float depth, int resolution, GenMethod method, const char* heightmapPath, const char* diffusePath, const char* grassPath, const char* snowPath, const char* normalPath);
    // 析构函数
    ~TerrainSandbox();

    // 设置OpenGL缓冲
    void setupMesh(Shader& shader);

    // 绘制沙盘
    void Draw(Shader& shader);

    // 根据世界坐标获取地形高度
    float getHeight(float worldX, float worldZ);

    // 获取地形尺寸
    float getTerrainWidth() const { return terrainWidth; }
    float getTerrainDepth() const { return terrainDepth; }

    // 在指定位置添加草地生长效果
    void addGrowth(float worldX, float worldZ);
    // 在指定位置添加积雪效果
    void addSnow(float worldX, float worldZ);

private:
    std::vector<TerrainVertex> vertices;
    unsigned int VAO, VBO, EBO;
    std::vector<unsigned int> indices;
    unsigned int diffuseTexture;
    unsigned int normalTexture;
    int indexCount;
    float baseHeight;

    unsigned int grassTexture;      // 草地纹理
    unsigned int snowTexture;       // 雪地纹理
    unsigned int growthTexture;     // 生长/积雪蒙版纹理
    unsigned int growthFBO;         // 用于绘制到生长纹理的FBO
    Shader paintShader;             // 绘制用的着色器
    unsigned int paintQuadVAO;      // 绘制笔刷用的VAO
    int growthTextureSize = 512;    // 生长纹理的分辨率

    // 存储地形尺寸和顶点数据以供查询
    float terrainWidth;
    float terrainDepth;
    int terrainResolution;
    std::vector<glm::vec3> terrainPositions;

    // 生成顶点数据
    void generateMesh(float width, float depth, int resolution, GenMethod method, const char* heightmapPath);

    // 加载纹理
    unsigned int loadTexture(const char* path);

    // 设置生长纹理
    void setupGrowthTexture();
};
