#include "lightning.h"
#include <iostream>
#include <glm/gtc/matrix_transform.hpp>
#include <cstdlib> // for rand()

Lightning::Lightning(Shader* shader, unsigned int textureID)
    : shader(shader),
      textureID(textureID),
      isLightningActive(false),
      lightningDuration(0.0f),
      newLightningStrike(false),
      lightningStrikePos(0.0f, 0.0f, 0.0f)
{
    setupMesh();
    resetTimer();
}

Lightning::~Lightning() {
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);
}

void Lightning::setupMesh() {
    // 一个简单的矩形板
    float vertices[] = {
        // positions      // texture Coords
        -0.5f, -0.5f, 0.0f,  0.0f, 0.0f,
         0.5f, -0.5f, 0.0f,  1.0f, 0.0f,
         0.5f,  0.5f, 0.0f,  1.0f, 1.0f,
        -0.5f,  0.5f, 0.0f,  0.0f, 1.0f
    };
    unsigned int indices[] = {
        0, 1, 2,
        2, 3, 0
    };

    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO); 
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    
    glBindVertexArray(0);
}

void Lightning::resetTimer() {
    // 随机设置下一次闪电的时间（例如 5 到 10 秒之间）
    timeToNextLightning = 5.0f + static_cast<float>(rand()) / (static_cast<float>(RAND_MAX / (10.0f - 5.0f)));
}

void Lightning::Update(float deltaTime, bool isRaining, TerrainSandbox& terrain) {
    if (isRaining) {
        if (isLightningActive) {
            // 闪电持续时间倒计时
            if (lightningDuration > 0.0f) {
                lightningDuration -= deltaTime;
            } else {
                isLightningActive = false;
            }
        } else if (timeToNextLightning > 0.0f) {
            // 下一次闪电倒计时
            timeToNextLightning -= deltaTime;
        } else {
            // 触发闪电
            isLightningActive = true;
            lightningDuration = 0.5f; // 闪电持续时间
            newLightningStrike = true;
            resetTimer();
        }
    } else {
        isLightningActive = false;
        resetTimer();
    }

    // 如果是新触发的闪电，计算随机位置
    if (newLightningStrike) {
        float halfW = terrain.getTerrainWidth() / 2.0f;
        float halfD = terrain.getTerrainDepth() / 2.0f;
        // 在沙盘范围内随机 X 和 Z
        float strikeX = -halfW + static_cast<float>(rand()) / (static_cast<float>(RAND_MAX / (halfW - (-halfW))));
        float strikeZ = -halfD + static_cast<float>(rand()) / (static_cast<float>(RAND_MAX / (halfD - (-halfD))));
        // 获取该位置的地形高度
        float strikeY = terrain.getHeight(strikeX, strikeZ);
        
        lightningStrikePos = glm::vec3(strikeX, strikeY, strikeZ);
        newLightningStrike = false;
    }
}

void Lightning::Draw(const glm::mat4& view, const glm::mat4& projection, const glm::vec3& cameraPos, const glm::vec3& cloudPos, const glm::vec3& sandboxWorldPos) {
    if (!isLightningActive || !shader) {
        return;
    }

    // 计算闪电的世界坐标
    glm::vec3 strikePosWorld = sandboxWorldPos + lightningStrikePos;
    // 闪电中心点（云和地面之间）
    glm::vec3 center = (cloudPos + strikePosWorld) / 2.0f;
    // 闪电高度
    float height = glm::distance(cloudPos, strikePosWorld);
    float width = height * 0.2f; // 宽度按比例

    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, center);
    model = glm::scale(model, glm::vec3(width, height, 1.0f));

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE); // 叠加混合模式让闪电更亮
    glDisable(GL_DEPTH_TEST); // 禁用深度测试让闪电不被云遮挡

    shader->use();
    shader->setMat4("projection", projection);
    shader->setMat4("view", view);
    shader->setMat4("model", model);
    shader->setVec3("cameraPos", cameraPos);
    
    glActiveTexture(GL_TEXTURE0);
    shader->setInt("lightningTexture", 0);
    glBindTexture(GL_TEXTURE_2D, textureID);

    glBindVertexArray(VAO);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);

    glEnable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); // 恢复默认混合
}