#include "sandbox_mesh.h"
// #define STB_IMAGE_IMPLEMENTATION 
#include "stb_image.h"           

TerrainSandbox::TerrainSandbox(float width, float depth, int resolution, GenMethod method, const char* heightmapPath, const char* diffusePath, const char* grassPath, const char* snowPath, const char* normalPath) 
    : paintShader("paint.vs", "paint.fs")
{
    baseHeight = 0.1f;
    // 保存地形尺寸信息
    terrainWidth = width;
    terrainDepth = depth;
    terrainResolution = resolution;

    generateMesh(width, depth, resolution, method, heightmapPath);
    // setupMesh();
    // 加载纹理
    diffuseTexture = diffusePath ? loadTexture(diffusePath) : 0;
    grassTexture = grassPath ? loadTexture(grassPath) : 0; 
    snowTexture = snowPath ? loadTexture(snowPath) : 0;
    normalTexture = normalPath ? loadTexture(normalPath) : 0;

    // 设置动态生长纹理
    setupGrowthTexture();
}

TerrainSandbox::~TerrainSandbox(){
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);
    glDeleteTextures(1, &diffuseTexture);
    glDeleteTextures(1, &normalTexture);
    glDeleteTextures(1, &grassTexture);
    glDeleteTextures(1, &snowTexture);
    glDeleteTextures(1, &growthTexture);
    glDeleteFramebuffers(1, &growthFBO);
    glDeleteVertexArrays(1, &paintQuadVAO);
}

void TerrainSandbox::generateMesh(float width, float depth, int resolution, GenMethod method, const char* heightmapPath) {
    // 生成地形网格的顶点位置的UV
    std::vector<glm::vec3> positions;
    std::vector<glm::vec2> uvs;
    std::vector<glm::vec3> normals;

    // 生成顶点位置和UV
    for (int i = 0; i <= resolution; i++) {
        for (int j = 0; j <= resolution; j++) {
            float x = (float)j / (float)resolution * width - width / 2.0f;
            float z = (float)i / (float)resolution * depth - depth / 2.0f;
            positions.push_back(glm::vec3(x, 0.0f, z)); // Y 坐标稍后设置
            uvs.push_back(glm::vec2((float)j / resolution, (float)i / resolution));
        }
    }

    // 根据方法设置高度 (Y坐标)
    if (method == GenMethod::HEIGHTMAP && heightmapPath != nullptr) {
        int imgWidth, imgHeight, nrChannels;
        unsigned char* data = stbi_load(heightmapPath, &imgWidth, &imgHeight, &nrChannels, 0);
        if (data) {
            for (int i = 0; i <= resolution; i++) {
                for (int j = 0; j <= resolution; j++) {
                    int texX = static_cast<int>(uvs[i * (resolution + 1) + j].x * (imgWidth - 1));
                    int texY = static_cast<int>(uvs[i * (resolution + 1) + j].y * (imgHeight - 1));
                    unsigned char height = data[(texY * imgWidth + texX) * nrChannels];
                    positions[i * (resolution + 1) + j].y = (height / 255.0f) * 0.2f;
                }
            }
            stbi_image_free(data);
        } else {
            std::cout << "Failed to load height map: " << heightmapPath << std::endl;
        }
    } else if (method == GenMethod::PROCEDURAL_RANDOM) {
        for (auto& pos : positions) {
            pos.y = ((rand() % 100) / 100.0f) * 0.15f;
        }
    }

    // 生成索引
    indices.clear();
    for (int i = 0; i < resolution; i++) {
        for (int j = 0; j < resolution; j++) {
            int row1 = i * (resolution + 1);
            int row2 = (i + 1) * (resolution + 1);
            indices.push_back(row1 + j);
            indices.push_back(row2 + j + 1);
            indices.push_back(row2 + j);
            indices.push_back(row1 + j);
            indices.push_back(row1 + j + 1);
            indices.push_back(row2 + j + 1);
        }
    }

    // 计算法线
    normals.assign(positions.size(), glm::vec3(0.0f));
    for (size_t i = 0; i < indices.size(); i += 3) {
        glm::vec3 p1 = positions[indices[i]];
        glm::vec3 p2 = positions[indices[i + 1]];
        glm::vec3 p3 = positions[indices[i + 2]];
        glm::vec3 normal = glm::normalize(glm::cross(p2 - p1, p3 - p1));
        normals[indices[i]] += normal;
        normals[indices[i + 1]] += normal;
        normals[indices[i + 2]] += normal;
    }
    for (auto& n : normals) {
        n = glm::normalize(n);
    }

    // 计算切线和副切线
    std::vector<glm::vec3> tangents(positions.size(), glm::vec3(0.0f));
    std::vector<glm::vec3> bitangents(positions.size(), glm::vec3(0.0f));
    for (size_t i = 0; i < indices.size(); i += 3) {
        glm::vec3 p1 = positions[indices[i]];
        glm::vec3 p2 = positions[indices[i+1]];
        glm::vec3 p3 = positions[indices[i+2]];
        glm::vec2 uv1 = uvs[indices[i]];
        glm::vec2 uv2 = uvs[indices[i+1]];
        glm::vec2 uv3 = uvs[indices[i+2]];

        glm::vec3 edge1 = p2 - p1;
        glm::vec3 edge2 = p3 - p1;
        glm::vec2 deltaUV1 = uv2 - uv1;
        glm::vec2 deltaUV2 = uv3 - uv1;

        float f = 1.0f / (deltaUV1.x * deltaUV2.y - deltaUV2.x * deltaUV1.y);
        if (isinf(f) || isnan(f)) f = 0.0f;

        glm::vec3 tangent, bitangent;
        tangent.x = f * (deltaUV2.y * edge1.x - deltaUV1.y * edge2.x);
        tangent.y = f * (deltaUV2.y * edge1.y - deltaUV1.y * edge2.y);
        tangent.z = f * (deltaUV2.y * edge1.z - deltaUV1.y * edge2.z);
        
        bitangent.x = f * (-deltaUV2.x * edge1.x + deltaUV1.x * edge2.x);
        bitangent.y = f * (-deltaUV2.x * edge1.y + deltaUV1.x * edge2.y);
        bitangent.z = f * (-deltaUV2.x * edge1.z + deltaUV1.x * edge2.z);

        tangents[indices[i]] += tangent;
        tangents[indices[i+1]] += tangent;
        tangents[indices[i+2]] += tangent;
        bitangents[indices[i]] += bitangent;
        bitangents[indices[i+1]] += bitangent;
        bitangents[indices[i+2]] += bitangent;
    }

    // 填充 Vertex 结构体 (for top surface)
    vertices.clear();
    vertices.resize(positions.size());
    for (size_t i = 0; i < positions.size(); ++i) {
        vertices[i].Position = positions[i];
        vertices[i].Normal = normals[i];
        vertices[i].TexCoords = uvs[i];
        vertices[i].Tangent = glm::normalize(tangents[i]);
        vertices[i].Bitangent = glm::normalize(bitangents[i]);
        vertices[i].isTopSurface = 1.0f;
    }

    // 生成底座和侧壁
    float halfW = width / 2.0f;
    float halfD = depth / 2.0f;
    
    unsigned int baseVertexOffset = vertices.size();
    // 为所有成员提供初始值
    glm::vec3 zeroVec(0.0f);
    vertices.push_back({{ halfW, -baseHeight,  halfD}, {0, -1, 0}, {1, 1}, zeroVec, zeroVec, 0.0f});
    vertices.push_back({{-halfW, -baseHeight,  halfD}, {0, -1, 0}, {0, 1}, zeroVec, zeroVec, 0.0f});
    vertices.push_back({{-halfW, -baseHeight, -halfD}, {0, -1, 0}, {0, 0}, zeroVec, zeroVec, 0.0f});
    vertices.push_back({{ halfW, -baseHeight, -halfD}, {0, -1, 0}, {1, 0}, zeroVec, zeroVec, 0.0f});
    
    indices.push_back(baseVertexOffset + 0);
    indices.push_back(baseVertexOffset + 1);
    indices.push_back(baseVertexOffset + 2);
    indices.push_back(baseVertexOffset + 0);
    indices.push_back(baseVertexOffset + 2);
    indices.push_back(baseVertexOffset + 3);

    auto addSideWall = [&](int idx1, int idx2, const glm::vec3& normal) {
        unsigned int currentOffset = vertices.size();
        
        glm::vec3 p_top1 = positions[idx1];
        glm::vec3 p_top2 = positions[idx2];
        glm::vec2 uv1 = uvs[idx1];
        glm::vec2 uv2 = uvs[idx2];

        glm::vec3 p_bottom1 = {p_top1.x, -baseHeight, p_top1.z};
        glm::vec3 p_bottom2 = {p_top2.x, -baseHeight, p_top2.z};

        // 为所有成员提供初始值
        vertices.push_back({p_top1,    normal, {uv1.x, uv1.y}, zeroVec, zeroVec, 0.0f});
        vertices.push_back({p_bottom1, normal, {uv1.x, 0.0f},  zeroVec, zeroVec, 0.0f});
        vertices.push_back({p_bottom2, normal, {uv2.x, 0.0f},  zeroVec, zeroVec, 0.0f});
        vertices.push_back({p_top2,    normal, {uv2.x, uv2.y}, zeroVec, zeroVec, 0.0f});

        indices.push_back(currentOffset + 0);
        indices.push_back(currentOffset + 1);
        indices.push_back(currentOffset + 2);
        indices.push_back(currentOffset + 0);
        indices.push_back(currentOffset + 2);
        indices.push_back(currentOffset + 3);
    };

    // 使用正确的索引逻辑生成四个侧壁
    int res = resolution;
    for (int i = 0; i < res; ++i) {
        // 后侧壁 (z 最小)
        addSideWall(i + 1, i, glm::vec3(0, 0, -1));
        // 前侧壁 (z 最大)
        addSideWall(res * (res + 1) + i, res * (res + 1) + i + 1, glm::vec3(0, 0, 1));
        // 左侧壁 (x 最小)
        addSideWall(i * (res + 1), (i + 1) * (res + 1), glm::vec3(-1, 0, 0));
        // 右侧壁 (x 最大)
        addSideWall((i + 1) * (res + 1) + res, i * (res + 1) + res, glm::vec3(1, 0, 0));
    }

    indexCount = indices.size();
    this->terrainPositions = positions; // 更新碰撞检测用的位置
}

void TerrainSandbox::setupMesh(Shader& shader) {
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(TerrainVertex), &vertices[0], GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), &indices[0], GL_STATIC_DRAW);

    // 使用glGetAttribLocation查询属性位置
    GLint posAttrib = glGetAttribLocation(shader.ID, "aPos");
    GLint normalAttrib = glGetAttribLocation(shader.ID, "aNormal");
    GLint texAttrib = glGetAttribLocation(shader.ID, "aTexCoords");
    GLint tangentAttrib = glGetAttribLocation(shader.ID, "aTangent");
    GLint bitangentAttrib = glGetAttribLocation(shader.ID, "aBitangent");
    GLint isTopAttrib = glGetAttribLocation(shader.ID, "aIsTopSurface"); 

    // 检查是否找到了属性
    if (posAttrib != -1) {
        glEnableVertexAttribArray(posAttrib);
        glVertexAttribPointer(posAttrib, 3, GL_FLOAT, GL_FALSE, sizeof(TerrainVertex), (void*)offsetof(TerrainVertex, Position));
    }
    if (normalAttrib != -1) {
        glEnableVertexAttribArray(normalAttrib);
        glVertexAttribPointer(normalAttrib, 3, GL_FLOAT, GL_FALSE, sizeof(TerrainVertex), (void*)offsetof(TerrainVertex, Normal));
    }
    if (texAttrib != -1) {
        glEnableVertexAttribArray(texAttrib);
        glVertexAttribPointer(texAttrib, 2, GL_FLOAT, GL_FALSE, sizeof(TerrainVertex), (void*)offsetof(TerrainVertex, TexCoords));
    }
    if (tangentAttrib != -1) {
        glEnableVertexAttribArray(tangentAttrib);
        glVertexAttribPointer(tangentAttrib, 3, GL_FLOAT, GL_FALSE, sizeof(TerrainVertex), (void*)offsetof(TerrainVertex, Tangent));
    }
    if (bitangentAttrib != -1) {
        glEnableVertexAttribArray(bitangentAttrib);
        glVertexAttribPointer(bitangentAttrib, 3, GL_FLOAT, GL_FALSE, sizeof(TerrainVertex), (void*)offsetof(TerrainVertex, Bitangent));
    }
    if (isTopAttrib != -1) {
        glEnableVertexAttribArray(isTopAttrib);
        glVertexAttribPointer(isTopAttrib, 1, GL_FLOAT, GL_FALSE, sizeof(TerrainVertex), (void*)offsetof(TerrainVertex, isTopSurface));
    }
    
    glBindVertexArray(0);
}

void TerrainSandbox::Draw(Shader& shader) {
    shader.use();
    
    // 绑定漫反射纹理到纹理单元 0
    glActiveTexture(GL_TEXTURE0);
    shader.setInt("texture_diffuse1", 0);
    glBindTexture(GL_TEXTURE_2D, diffuseTexture);

    // 绑定草地和生长蒙版纹理 1、2
    glActiveTexture(GL_TEXTURE1);
    shader.setInt("texture_grass1", 1);
    glBindTexture(GL_TEXTURE_2D, grassTexture);

    // 绑定雪地纹理
    glActiveTexture(GL_TEXTURE2);
    shader.setInt("texture_snow1", 2);
    glBindTexture(GL_TEXTURE_2D, snowTexture);

    glActiveTexture(GL_TEXTURE3);
    shader.setInt("texture_effect_mask", 3);
    glBindTexture(GL_TEXTURE_2D, growthTexture);

    // 绑定法线贴图到纹理单元4
    if (normalTexture != 0) {
        glActiveTexture(GL_TEXTURE4);
        shader.setInt("texture_normal1", 4);
        glBindTexture(GL_TEXTURE_2D, normalTexture);
    }

    glBindVertexArray(VAO);
    glDrawElements(GL_TRIANGLES, indexCount, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);

    glActiveTexture(GL_TEXTURE0);
}

unsigned int TerrainSandbox::loadTexture(const char* path) {
    unsigned int textureID;
    glGenTextures(1, &textureID);

    int width, height, nrComponents;
    unsigned char* data = stbi_load(path, &width, &height, &nrComponents, 0);
    if (data) {
        std::cout << "SUCCESS: Texture loaded successfully from path: " << path << std::endl;

        GLenum format;
        if (nrComponents == 1) format = GL_RED;
        else if (nrComponents == 3) format = GL_RGB;
        else if (nrComponents == 4) format = GL_RGBA;

        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        stbi_image_free(data);
    }
    else {
        std::cout << "ERROR: Texture failed to load at path: " << path << std::endl;
        // stbi_image_free(data);
    }

    return textureID;
}

// getHeight 函数的实现
float barryCentric(glm::vec3 p1, glm::vec3 p2, glm::vec3 p3, glm::vec2 pos) {
    float det = (p2.z - p3.z) * (p1.x - p3.x) + (p3.x - p2.x) * (p1.z - p3.z);
    // 增加一个除零保护
    if (abs(det) < 1e-6) return p1.y;
    float l1 = ((p2.z - p3.z) * (pos.x - p3.x) + (p3.x - p2.x) * (pos.y - p3.z)) / det;
    float l2 = ((p3.z - p1.z) * (pos.x - p3.x) + (p1.x - p3.x) * (pos.y - p3.z)) / det;
    float l3 = 1.0f - l1 - l2;
    return l1 * p1.y + l2 * p2.y + l3 * p3.y;
}

float TerrainSandbox::getHeight(float worldX, float worldZ) {
    // 将世界坐标转换为地形的局部坐标 (0-1范围)
    float terrainX = (worldX / terrainWidth) + 0.5f;
    float terrainZ = (worldZ / terrainDepth) + 0.5f;

    // 边界检查
    if (terrainX < 0.0f || terrainX > 1.0f || terrainZ < 0.0f || terrainZ > 1.0f) {
        return -1000.0f; 
    }

    // 计算在哪个网格单元
    int gridX = static_cast<int>(floor(terrainX * terrainResolution));
    int gridZ = static_cast<int>(floor(terrainZ * terrainResolution));

    // 索引安全检查
    if (gridX >= terrainResolution || gridZ >= terrainResolution || gridX < 0 || gridZ < 0) {
        return -1000.0f;
    }

    // 计算在单元格内的坐标 (0-1范围)
    float xCoord = fmod(terrainX * terrainResolution, 1.0f);
    float zCoord = fmod(terrainZ * terrainResolution, 1.0f);

    // 获取四个角的顶点
    int res = terrainResolution;
    glm::vec3 p1 = terrainPositions[(gridZ * (res + 1)) + gridX];
    glm::vec3 p2 = terrainPositions[(gridZ * (res + 1)) + gridX + 1];
    glm::vec3 p3 = terrainPositions[((gridZ + 1) * (res + 1)) + gridX];
    glm::vec3 p4 = terrainPositions[((gridZ + 1) * (res + 1)) + gridX + 1];

    // 进行重心插值
    // 我们需要将单元格内的坐标 (xCoord, zCoord) 转换为与 p1,p2,p3 相同的局部模型坐标系
    float interpolatedX = p1.x + (p2.x - p1.x) * xCoord;
    float interpolatedZ = p1.z + (p3.z - p1.z) * zCoord;
    glm::vec2 localPos(interpolatedX, interpolatedZ);

    // 根据在哪个三角形中进行重心插值
    if (xCoord + zCoord < 1) { // 左上三角形 (p1, p2, p3)
        return barryCentric(p1, p2, p3, localPos);
    } else { // 右下三角形 (p2, p4, p3)
        return barryCentric(p2, p4, p3, localPos);
    }
}

void TerrainSandbox::setupGrowthTexture() {
    // 创建生长纹理
    glGenTextures(1, &growthTexture);
    glBindTexture(GL_TEXTURE_2D, growthTexture);
    // 创建一个 512x512 的空白纹理，初始为黑色
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, growthTextureSize, growthTextureSize, 0, GL_RGBA, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    // 创建FBO并绑定纹理
    glGenFramebuffers(1, &growthFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, growthFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, growthTexture, 0);

    // 检查FBO是否完整
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        std::cout << "ERROR::FRAMEBUFFER:: Framebuffer is not complete!" << std::endl;
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // 创建一个简单的四边形用于绘制笔刷
    float quadVertices[] = { -1.0f, 1.0f, -1.0f, -1.0f, 1.0f, 1.0f, 1.0f, -1.0f };
    unsigned int quadVBO;
    glGenVertexArrays(1, &paintQuadVAO);
    glGenBuffers(1, &quadVBO);
    glBindVertexArray(paintQuadVAO);
    glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glBindVertexArray(0);
}

void TerrainSandbox::addGrowth(float worldX, float worldZ) {
    const unsigned int SCR_WIDTH = 1600;
    const unsigned int SCR_HEIGHT = 1200;
    // 将世界坐标转换为地形UV坐标 (0-1范围)
    float u = (worldX / terrainWidth) + 0.5f;
    float v = (worldZ / terrainDepth) + 0.5f;

    // 如果在范围外则忽略
    if (u < 0 || u > 1 || v < 0 || v > 1) return;

    // 开始绘制到 growthTexture
    glViewport(0, 0, growthTextureSize, growthTextureSize);
    glBindFramebuffer(GL_FRAMEBUFFER, growthFBO);
    
    // 启用混合，这样每次绘制都是叠加效果
    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE); // 加法混合

    paintShader.use();
    paintShader.setVec2("center", glm::vec2(u, v));
    paintShader.setFloat("radius", 0.15f); // 草地斑块的半径
    paintShader.setFloat("scale", (float)growthTextureSize);
    // 绘制到红色通道
    paintShader.setVec3("paintColor", glm::vec3(1.0f, 0.0f, 0.0f)); 

    glBindVertexArray(paintQuadVAO);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glBindVertexArray(0);

    glDisable(GL_BLEND);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // 恢复主视口
    glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT); 
}

void TerrainSandbox::addSnow(float worldX, float worldZ) {
    const unsigned int SCR_WIDTH = 1600;
    const unsigned int SCR_HEIGHT = 1200;

    float u = (worldX / terrainWidth) + 0.5f;
    float v = (worldZ / terrainDepth) + 0.5f;

    if (u < 0 || u > 1 || v < 0 || v > 1) return;

    glViewport(0, 0, growthTextureSize, growthTextureSize);
    glBindFramebuffer(GL_FRAMEBUFFER, growthFBO);
    
    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE);

    paintShader.use();
    paintShader.setVec2("center", glm::vec2(u, v));
    paintShader.setFloat("radius", 0.3f); 
    paintShader.setFloat("scale", (float)growthTextureSize);
    // 绘制到绿色通道
    paintShader.setVec3("paintColor", glm::vec3(0.0f, 0.5f, 0.0f)); 

    glBindVertexArray(paintQuadVAO);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glBindVertexArray(0);

    glDisable(GL_BLEND);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);
}