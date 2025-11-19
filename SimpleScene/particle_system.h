#pragma once

#include <vector>
#include <glad/glad.h>
#include <glm/glm.hpp>
#include "shader.h"
#include "sandbox_mesh.h"

// 定义单个粒子的结构
struct Particle {
    glm::vec3 position;
    glm::vec3 velocity;
    float life;
    glm::vec3 initialOffset; // 存储粒子相对于云中心的初始偏移
};

// 粒子系统类
class ParticleSystem {
public:
    // 效果类型
    enum class EffectType { NONE, GROWTH, SNOW }; 

    // 构造函数：需要最大粒子数和指向地形的指针
    ParticleSystem(unsigned int maxParticles, TerrainSandbox* terrain, const char* vsPath = "rain.vs", const char* fsPath = "rain.fs");
    ~ParticleSystem();

    // 更新所有粒子的状态
    void Update(float dt, const glm::vec3& cloudCenter, const glm::vec3& terrainWorldPos, EffectType effect);
    
    // 渲染所有粒子
    void Draw(const glm::mat4& view, const glm::mat4& projection, bool drawAsPoints = false);

private:
    // 重置单个粒子，让它回到云中
    void resetParticle(Particle& particle, const glm::vec3& cloudCenter);

    std::vector<Particle> particles;
    unsigned int maxParticles;
    TerrainSandbox* terrain; // 用于碰撞检测的地形指针

    unsigned int VAO, VBO;
    Shader particleShader;
};