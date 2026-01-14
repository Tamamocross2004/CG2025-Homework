#pragma once

#include <vector>
#include <glad/glad.h>
#include <glm/glm.hpp>
#include "shader.h"
#include "model.h"

// 定义单个箭头的结构
struct Arrow {
    glm::vec3 position;
    glm::vec3 velocity;
    float life;           // 生命周期（秒）
    float rotationY;       // Y轴旋转角度
    bool active;          // 是否激活
    bool stuck;           // 是否插在表面上（碰撞后停止移动但继续渲染）
};

// 箭头系统类
class ArrowSystem {
public:
    // 构造函数
    ArrowSystem(Model* arrowModel, Shader* shader, const glm::vec3& spawnPosition);
    ~ArrowSystem();

    // 更新所有箭头的状态
    void Update(float dt);
    
    // 渲染所有箭头
    void Draw(const glm::mat4& view, const glm::mat4& projection, const glm::vec3& lightPos, const glm::vec3& viewPos, const glm::vec3& lightColor);
    
    // 开始/停止发射
    void StartShooting();
    void StopShooting();
    bool IsShooting() const { return isShooting; }
    
    // 设置发射参数
    void SetSpawnPosition(const glm::vec3& pos) { spawnPosition = pos; }
    void SetSpawnDirection(const glm::vec3& dir) { spawnDirection = dir; }
    
    // 设置旋转角度偏移（用于调整箭头模型的朝向）
    // rotationX, rotationY, rotationZ 分别对应绕X、Y、Z轴的旋转角度（度）
    void SetRotationOffset(float rotX, float rotY, float rotZ) {
        rotationOffsetX = rotX;
        rotationOffsetY = rotY;
        rotationOffsetZ = rotZ;
    }

private:
    // 重置单个箭头，让它从发射位置重新开始
    void resetArrow(Arrow& arrow);
    
    std::vector<Arrow> arrows;
    unsigned int maxArrows;
    Model* arrowModel;
    Shader* arrowShader;
    
    glm::vec3 spawnPosition;      // 发射位置
    glm::vec3 spawnDirection;      // 发射方向（归一化）
    float arrowSpeed;              // 箭头速度
    float spawnInterval;           // 发射间隔（秒）
    float timeSinceLastSpawn;      // 距离上次发射的时间
    bool isShooting;               // 是否正在发射
    
    float arrowLifetime;            // 箭头生命周期（秒）
    
    // 旋转角度偏移（用于调整不同方向箭头的模型朝向）
    float rotationOffsetX;          // 绕X轴旋转偏移（度）
    float rotationOffsetY;          // 绕Y轴旋转偏移（度）
    float rotationOffsetZ;          // 绕Z轴旋转偏移（度）
};

