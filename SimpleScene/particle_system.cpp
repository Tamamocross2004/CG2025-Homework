#include "particle_system.h"
#include <glm/gtc/random.hpp>

ParticleSystem::ParticleSystem(unsigned int maxParticles, TerrainSandbox* terrain)
    : maxParticles(maxParticles), terrain(terrain), particleShader("rain.vs", "rain.fs") {
    
    particles.resize(maxParticles);

    // 初始化所有粒子，让它们在第一次下雨时被重置
    for (auto& p : particles) {
        p.life = 0.0f; // 使用 life=0 表示粒子是“死的”
    }

    // 创建 VBO 和 VAO
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);

    // 为粒子位置数据预留空间
    glBufferData(GL_ARRAY_BUFFER, maxParticles * sizeof(glm::vec3), NULL, GL_STREAM_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);
    glBindVertexArray(0);
}

ParticleSystem::~ParticleSystem() {
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
}

void ParticleSystem::resetParticle(Particle& particle, const glm::vec3& cloudCenter) {
    // 在云的范围内随机生成位置
    float randX = glm::linearRand(-0.5f, 0.5f);    // X轴范围
    float randZ = glm::linearRand(-0.25f, 0.25f); // Z轴范围
    particle.position = cloudCenter + glm::vec3(randX, 0.0f, randZ);

    // 设置一个随机的下落速度
    float gravity = 9.8f;
    particle.velocity = glm::vec3(0.0f, -gravity * glm::linearRand(0.5f, 1.5f), 0.0f);
    particle.life = 1.0f;
}

void ParticleSystem::Update(float dt, const glm::vec3& cloudCenter, const glm::vec3& terrainWorldPos){
    for(auto& p : particles){
        if(p.life <= 0.0f){
            resetParticle(p, cloudCenter);
        }
        // 更新位置
        p.position += p.velocity * dt;

        // 碰撞检测
        // 将雨滴的世界坐标转换为沙盘的局部坐标
        glm::vec3 particleLocalPos = p.position - terrainWorldPos;

        // 获取沙盘的尺寸信息
        float terrainWidth = terrain->getTerrainWidth();
        float terrainDepth = terrain->getTerrainDepth();
        float halfW = terrainWidth / 2.0f;
        float halfD = terrainDepth / 2.0f;

        // 检查雨滴是否在沙盘的 XZ 平面范围内
        if (particleLocalPos.x >= -halfW && particleLocalPos.x <= halfW &&
            particleLocalPos.z >= -halfD && particleLocalPos.z <= halfD)
        {
            // 在范围内，进行精确的地形高度碰撞检测
            float localTerrainHeight = terrain->getHeight(particleLocalPos.x, particleLocalPos.z);
            
            // 计算地形表面在世界坐标系中的实际高度
            float worldTerrainSurfaceY = terrainWorldPos.y + localTerrainHeight;

            if (p.position.y < worldTerrainSurfaceY) {
                p.life = 0.0f; // 碰到地形，重置
            }
        }
        else
        {
            // 在范围外，检查是否碰到了沙盘底座的顶面
            // 沙盘顶面的局部 Y 坐标是 0
            if (p.position.y < terrainWorldPos.y) {
                 p.life = 0.0f; // 碰到沙盘范围外的顶面，重置
            }
        }

        // 增加一个最低高度检查，防止无限下落
        if (p.position.y < -5.0f) {
            p.life = 0.0f;
        }
    }

}

void ParticleSystem::Draw(const glm::mat4& view, const glm::mat4& projection) {
    // 启用混合和点大小设置
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_PROGRAM_POINT_SIZE);

    // 收集所有“活的”粒子的位置
    std::vector<glm::vec3> particle_positions;
    particle_positions.reserve(maxParticles);
    for (const auto& p : particles) {
        if (p.life > 0.0f) {
            particle_positions.push_back(p.position);
        }
    }

    if (!particle_positions.empty()) {
        // 更新VBO数据
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0, particle_positions.size() * sizeof(glm::vec3), particle_positions.data());

        // 绘制雨滴
        particleShader.use();
        particleShader.setMat4("projection", projection);
        particleShader.setMat4("view", view);
        glBindVertexArray(VAO);
        glDrawArrays(GL_POINTS, 0, particle_positions.size());
        glBindVertexArray(0);
    }

    glDisable(GL_PROGRAM_POINT_SIZE);
    glDisable(GL_BLEND);
}