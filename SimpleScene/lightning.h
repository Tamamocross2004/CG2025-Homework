#pragma once
#include "shader.h"
#include "sandbox_mesh.h"
#include <glm/glm.hpp>

class Lightning {
public:
    // 构造函数：接收外部加载好的 Shader 和 纹理ID
    Lightning(Shader* shader, unsigned int textureID);
    
    ~Lightning();

    // 更新闪电状态（计时器、位置随机化）
    void Update(float deltaTime, bool isRaining, TerrainSandbox& terrain);

    // 绘制闪电
    void Draw(const glm::mat4& view, const glm::mat4& projection, const glm::vec3& cameraPos, const glm::vec3& cloudPos, const glm::vec3& sandboxWorldPos);

    // 用于外部查询是否正在打雷（用于改变环境光照）
    bool IsActive() const { return isLightningActive; }

private:
    bool isLightningActive;
    float lightningDuration;
    float timeToNextLightning;
    bool newLightningStrike;
    glm::vec3 lightningStrikePos;

    Shader* shader;         // 借用的指针，不负责删除
    unsigned int textureID; // 借用的ID，不负责删除
    unsigned int VAO, VBO, EBO;

    void setupMesh();
    void resetTimer();
};