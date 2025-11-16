#include "sandbox_mesh.h"
// #define STB_IMAGE_IMPLEMENTATION 
#include "stb_image.h"           

TerrainSandbox::TerrainSandbox(float width, float depth, int resolution, GenMethod method, const char* heightmapPath, const char* diffusePath) {
    baseHeight = 0.1f;
    generateMesh(width, depth, resolution, method, heightmapPath);
    setupMesh();
    if(diffusePath) {
        diffuseTexture = loadTexture(diffusePath);
    } else {
        diffuseTexture = 0;
    }
}

TerrainSandbox::~TerrainSandbox(){
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);
    if(diffuseTexture != 0){
        glDeleteTextures(1, &diffuseTexture);
    }
}

void TerrainSandbox::generateMesh(float width, float depth, int resolution, GenMethod method, const char* heightmapPath) {
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
                    // 根据UV坐标采样高度图
                    int texX = static_cast<int>(uvs[i * (resolution + 1) + j].x * (imgWidth - 1));
                    int texY = static_cast<int>(uvs[i * (resolution + 1) + j].y * (imgHeight - 1));
                    unsigned char height = data[(texY * imgWidth + texX) * nrChannels];
                    positions[i * (resolution + 1) + j].y = (height / 255.0f) * 0.2f; // 高度缩放因子
                }
            }
            stbi_image_free(data);
        } else {
            std::cout << "Failed to load height map: " << heightmapPath << std::endl;
        }
    } else if (method == GenMethod::PROCEDURAL_RANDOM) {
        for (auto& pos : positions) {
            pos.y = ((rand() % 100) / 100.0f) * 0.15f; // 随机高度
        }
    }

    // 生成索引
    for (int i = 0; i < resolution; i++) {
        for (int j = 0; j < resolution; j++) {
            int row1 = i * (resolution + 1);
            int row2 = (i + 1) * (resolution + 1);
            // 三角形 1
            indices.push_back(row1 + j);
            indices.push_back(row2 + j + 1);
            indices.push_back(row2 + j);
            // 三角形 2
            indices.push_back(row1 + j);
            indices.push_back(row1 + j + 1);
            indices.push_back(row2 + j + 1);
        }
    }
    indexCount = indices.size();

    // 计算法线
    normals.resize(positions.size(), glm::vec3(0.0f));
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

    // 合并顶点数据
    for (size_t i = 0; i < positions.size(); ++i) {
        vertices.push_back(positions[i].x);
        vertices.push_back(positions[i].y);
        vertices.push_back(positions[i].z);
        vertices.push_back(normals[i].x);
        vertices.push_back(normals[i].y);
        vertices.push_back(normals[i].z);
        vertices.push_back(uvs[i].x);
        vertices.push_back(uvs[i].y);
    }

    // --- 在这里添加生成底座的代码 ---
    float halfW = width / 2.0f;
    float halfD = depth / 2.0f;
    unsigned int baseVertexOffset = positions.size(); // 地形顶点之后的偏移量

    // 底座的顶点位置
    std::vector<glm::vec3> base_positions = {
        // 底面 (y = -baseHeight)
        { halfW, -baseHeight,  halfD}, {-halfW, -baseHeight,  halfD}, {-halfW, -baseHeight, -halfD}, { halfW, -baseHeight, -halfD},
        // 前侧面 (z = halfD)
        { halfW,  0.0f,  halfD}, {-halfW,  0.0f,  halfD},
        // 后侧面 (z = -halfD)
        {-halfW,  0.0f, -halfD}, { halfW,  0.0f, -halfD},
        // 右侧面 (x = halfW)
        { halfW,  0.0f, -halfD},
        // 左侧面 (x = -halfW)
        {-halfW,  0.0f,  halfD}
    };

    // 底座的索引
    std::vector<unsigned int> base_indices = {
        // 底面
        baseVertexOffset + 0, baseVertexOffset + 1, baseVertexOffset + 2, 
        baseVertexOffset + 0, baseVertexOffset + 2, baseVertexOffset + 3,
        // 前侧面
        baseVertexOffset + 4, baseVertexOffset + 1, baseVertexOffset + 5, 
        baseVertexOffset + 4, baseVertexOffset + 0, baseVertexOffset + 1,
        // 后侧面
        baseVertexOffset + 6, baseVertexOffset + 3, baseVertexOffset + 7, 
        baseVertexOffset + 6, baseVertexOffset + 2, baseVertexOffset + 3,
        // 右侧面
        baseVertexOffset + 8, baseVertexOffset + 0, baseVertexOffset + 3, 
        baseVertexOffset + 8, baseVertexOffset + 4, baseVertexOffset + 0,
        // 左侧面
        baseVertexOffset + 9, baseVertexOffset + 2, baseVertexOffset + 1, 
        baseVertexOffset + 9, baseVertexOffset + 6, baseVertexOffset + 2
    };

    // 底座的法线
    std::vector<glm::vec3> base_normals;
    base_normals.resize(base_positions.size(), glm::vec3(0.0f));
    for (size_t i = 0; i < base_indices.size(); i += 3) {
        glm::vec3 p1 = base_positions[base_indices[i] - baseVertexOffset];
        glm::vec3 p2 = base_positions[base_indices[i + 1] - baseVertexOffset];
        glm::vec3 p3 = base_positions[base_indices[i + 2] - baseVertexOffset];
        glm::vec3 normal = glm::normalize(glm::cross(p2 - p1, p3 - p1));
        base_normals[base_indices[i] - baseVertexOffset] += normal;
        base_normals[base_indices[i + 1] - baseVertexOffset] += normal;
        base_normals[base_indices[i + 2] - baseVertexOffset] += normal;
    }
    for (auto& n : base_normals) {
        n = glm::normalize(n);
    }
    
    // 底座的UVs (简单映射)
    std::vector<glm::vec2> base_uvs = {
        {1.0f, 1.0f}, {0.0f, 1.0f}, {0.0f, 0.0f}, {1.0f, 0.0f}, // 底面
        {1.0f, 1.0f}, {0.0f, 1.0f}, // 前侧面
        {0.0f, 0.0f}, {1.0f, 0.0f}, // 后侧面
        {1.0f, 0.0f}, // 右侧面
        {0.0f, 1.0f}  // 左侧面
    };

    // 将底座数据合并到主顶点和索引列表中
    for (size_t i = 0; i < base_positions.size(); ++i) {
        vertices.push_back(base_positions[i].x);
        vertices.push_back(base_positions[i].y);
        vertices.push_back(base_positions[i].z);
        vertices.push_back(base_normals[i].x);
        vertices.push_back(base_normals[i].y);
        vertices.push_back(base_normals[i].z);
        vertices.push_back(base_uvs[i].x);
        vertices.push_back(base_uvs[i].y);
    }
    indices.insert(indices.end(), base_indices.begin(), base_indices.end());
    indexCount = indices.size();
}

void TerrainSandbox::setupMesh() {
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), &vertices[0], GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), &indices[0], GL_STATIC_DRAW);

    // 位置属性
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);

    // 法线属性
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));

    // 纹理坐标属性
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));

    glBindVertexArray(0);
}

void TerrainSandbox::Draw(Shader& shader) {
    shader.use();
    if (diffuseTexture != 0) {
        glActiveTexture(GL_TEXTURE0);
        shader.setInt("texture_diffuse1", 0);
        glBindTexture(GL_TEXTURE_2D, diffuseTexture);
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
        std::cout << "Texture failed to load at path: " << path << std::endl;
        // stbi_image_free(data);
    }

    return textureID;
}