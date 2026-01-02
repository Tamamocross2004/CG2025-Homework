#include "arrow_system.h"
#include <glm/gtc/random.hpp>
#include <algorithm>
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

ArrowSystem::ArrowSystem(Model* arrowModel, Shader* shader, const glm::vec3& spawnPosition)
    : arrowModel(arrowModel), arrowShader(shader), spawnPosition(spawnPosition),
      maxArrows(100), arrowSpeed(10.0f), spawnInterval(0.01f), timeSinceLastSpawn(0.0f),  
      isShooting(false), arrowLifetime(5.0f),  // 增加生命周期到5秒
      rotationOffsetX(0.0f), rotationOffsetY(0.0f), rotationOffsetZ(90.0f) {  // 默认绕Z轴旋转90度
    
    // 默认发射方向（朝向房间内部，即负X方向）
    spawnDirection = glm::normalize(glm::vec3(-1.0f, 0.0f, 0.0f));
    
    // 初始化所有箭头
    arrows.resize(maxArrows);
    for (auto& arrow : arrows) {
        arrow.active = false;
        arrow.life = 0.0f;
    }
    
    std::cout << "ArrowSystem created at position: (" 
              << spawnPosition.x << ", " << spawnPosition.y << ", " << spawnPosition.z << ")" << std::endl;
}

ArrowSystem::~ArrowSystem() {
    // Model和Shader由外部管理，这里不需要删除
}

void ArrowSystem::resetArrow(Arrow& arrow) {
    arrow.position = spawnPosition;
    
    // 将发射位置稍微向前移动，确保箭头从发射点前面发射（避免被墙遮挡）
    // 根据发射方向决定偏移方向
    float offsetAmount = 0.1f;
    arrow.position += spawnDirection * offsetAmount;  // 沿发射方向向前移动
    
    // 添加一些随机偏移，使箭头不完全重叠
    glm::vec3 randomOffset = glm::vec3(
        glm::linearRand(-0.05f, 0.05f),
        glm::linearRand(-0.05f, 0.05f),
        glm::linearRand(-0.05f, 0.05f)
    );
    arrow.position += randomOffset;
    
    // 速度方向：主要朝向spawnDirection（x轴负方向），添加一些随机性
    glm::vec3 baseDir = spawnDirection;  // 已经是(-1, 0, 0)，即x轴负方向
    glm::vec3 randomDir = glm::normalize(glm::vec3(
        glm::linearRand(-0.1f, 0.1f),   // x方向随机性较小，保持主要朝向x轴负方向
        glm::linearRand(-0.1f, 0.2f),   // 稍微向上
        glm::linearRand(-0.2f, 0.2f)    // z方向可以有一些随机性
    ));
    arrow.velocity = glm::normalize(baseDir + randomDir * 0.2f) * arrowSpeed;  // 减少随机性
    
    // 计算Y轴旋转角度（基于速度方向，使箭头朝向x轴负方向）
    // 对于x轴负方向，在XZ平面上的角度应该是180度
    glm::vec2 velXZ(arrow.velocity.x, arrow.velocity.z);
    if (glm::length(velXZ) > 0.001f) {
        // atan2(x, z) 计算从z轴正方向到(x,z)的角度
        // 对于x轴负方向(-1, 0)，应该是-90度或270度
        // 但我们希望箭头朝向x轴负方向，所以需要调整
        arrow.rotationY = atan2f(arrow.velocity.x, arrow.velocity.z) * 180.0f / static_cast<float>(M_PI);  // 手动转换为度
    } else {
        // 如果速度主要在y方向，默认朝向x轴负方向（180度）
        arrow.rotationY = 180.0f;
    }
    
    arrow.life = arrowLifetime;
    arrow.active = true;
}

void ArrowSystem::Update(float dt) {
    // 如果正在发射，尝试生成新箭头
    if (isShooting) {
        timeSinceLastSpawn += dt;
        if (timeSinceLastSpawn >= spawnInterval) {
            // 查找一个未激活的箭头
            bool found = false;
            for (auto& arrow : arrows) {
                if (!arrow.active) {
                    resetArrow(arrow);
                    timeSinceLastSpawn = 0.0f;
                    found = true;
                    break;
                }
            }
            if (!found) {
                // 如果所有箭头都在使用，输出警告
                static int warningCount = 0;
                if (warningCount++ < 5) {
                    std::cout << "Warning: All arrows are active, cannot spawn new arrow" << std::endl;
                }
            }
        }
    }
    
    // 统计激活的箭头数量（用于调试）
    static int frameCount = 0;
    if (++frameCount % 60 == 0) {  // 每60帧输出一次
        int activeCount = 0;
        for (const auto& arrow : arrows) {
            if (arrow.active) activeCount++;
        }
        if (activeCount > 0) {
            std::cout << "Active arrows: " << activeCount << std::endl;
        }
    }
    
    // 更新所有激活的箭头
    for (auto& arrow : arrows) {
        if (arrow.active) {
            // 更新位置
            arrow.position += arrow.velocity * dt;
            
            // 更新生命周期
            arrow.life -= dt;
            
            // 碰撞检测：左墙（x负半轴）
            // 左墙中心在x=-4.0，厚度0.1，内表面在x=-3.95左右
            // 检测箭矢是否碰到左墙内表面（使用稍微宽松的阈值-3.9，确保能检测到碰撞）
            const float LEFT_WALL_X = -3.9f;  // 左墙内表面x坐标阈值
            if (arrow.position.x <= LEFT_WALL_X) {
                arrow.active = false;
                continue;  // 箭矢已消失，跳过后续检测
            }
            
            // 如果生命周期结束或超出房间范围，停用箭头
            if (arrow.life <= 0.0f || 
                arrow.position.x < -5.0f || arrow.position.x > 5.0f ||
                arrow.position.y < -5.0f || arrow.position.y > 5.0f ||
                arrow.position.z < -5.0f || arrow.position.z > 5.0f) {
                arrow.active = false;
            }
        }
    }
}

void ArrowSystem::Draw(const glm::mat4& view, const glm::mat4& projection, 
                       const glm::vec3& lightPos, const glm::vec3& viewPos, 
                       const glm::vec3& lightColor) {
    if (!arrowModel || !arrowShader) {
        static bool warned = false;
        if (!warned) {
            std::cout << "Warning: ArrowSystem cannot draw - missing model or shader" << std::endl;
            warned = true;
        }
        return;
    }
    
    arrowShader->use();
    arrowShader->setMat4("projection", projection);
    arrowShader->setMat4("view", view);
    arrowShader->setVec3("viewPos", viewPos);
    arrowShader->setVec3("light.position", lightPos);
    arrowShader->setVec3("light.ambient", lightColor * 0.2f);
    arrowShader->setVec3("light.diffuse", lightColor);
    arrowShader->setVec3("light.specular", lightColor * 0.2f);
    
    // 设置台灯点光源（与模型渲染保持一致）
    // 注意：这里需要从外部传入lampLightPos，暂时设为默认值
    // 如果需要台灯光照，可以在main.cpp中调用时传入
    arrowShader->setBool("lampOn", false); // 箭头不受台灯影响，或可以从外部传入
    arrowShader->setBool("isLampModel", false);
    
    // 绘制所有激活的箭头
    int drawnCount = 0;
    for (const auto& arrow : arrows) {
        if (arrow.active) {
            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model, arrow.position);
            
            // 如果箭头现在朝向y轴负方向，需要旋转使其朝向x轴负方向
            // 从y轴负方向(0,-1,0)到x轴负方向(-1,0,0)的旋转：
            // 方法1：先绕z轴旋转90度（y负->x正），然后绕y轴旋转180度（x正->x负）
            // 但这样箭头会垂直，需要先让箭头水平
            // 方法2：先绕x轴旋转90度（y负->z正），然后绕y轴旋转90度（z正->x负）
            
            // 应用旋转偏移（用于调整不同方向箭头的模型朝向）
            // 使用成员变量中的旋转偏移量，允许不同箭头系统有不同的旋转
            model = glm::rotate(model, glm::radians(rotationOffsetX), glm::vec3(1.0f, 0.0f, 0.0f));  
            model = glm::rotate(model, glm::radians(rotationOffsetY), glm::vec3(0.0f, 1.0f, 0.0f));  
            model = glm::rotate(model, glm::radians(rotationOffsetZ), glm::vec3(0.0f, 0.0f, 1.0f));  

            // 根据速度方向进行微调（考虑z和y方向的偏移）
            // 计算在XZ平面上的旋转角度（相对于x轴负方向）
            if (glm::abs(arrow.velocity.z) > 0.01f) {
                // 速度在XZ平面上的投影
                glm::vec2 velXZ(arrow.velocity.x, arrow.velocity.z);
                // 计算相对于x轴负方向的角度
                float angle = atan2f(velXZ.y, -velXZ.x) * 180.0f / static_cast<float>(M_PI);
                if (glm::abs(angle) > 1.0f) {  // 只有角度足够大时才旋转
                    model = glm::rotate(model, glm::radians(angle), glm::vec3(0.0f, 1.0f, 0.0f));
                }
            }
            
            // 根据y方向的速度进行上下倾斜（俯仰角）
            if (glm::abs(arrow.velocity.y) > 0.01f) {
                float horizontalSpeed = glm::length(glm::vec2(arrow.velocity.x, arrow.velocity.z));
                float pitchAngle = atan2f(arrow.velocity.y, horizontalSpeed) * 180.0f / static_cast<float>(M_PI);
                if (glm::abs(pitchAngle) > 1.0f) {
                    // 绕z轴旋转（在XZ平面内倾斜）
                    model = glm::rotate(model, glm::radians(pitchAngle), glm::vec3(0.0f, 0.0f, 1.0f));
                }
            }
            
            // 缩放箭头（增大尺寸以便观察）
            model = glm::scale(model, glm::vec3(0.5f)); // 增大到0.5倍，更容易看到
            
            arrowShader->setMat4("model", model);
            arrowModel->Draw(*arrowShader);
            drawnCount++;
        }
    }
    
    // 调试输出（每60帧输出一次）
    static int drawFrameCount = 0;
    if (++drawFrameCount % 60 == 0 && drawnCount > 0) {
        std::cout << "Drawing " << drawnCount << " arrows" << std::endl;
    }
}

void ArrowSystem::StartShooting() {
    isShooting = true;
    timeSinceLastSpawn = 0.0f;
    std::cout << "ArrowSystem: StartShooting called, spawn position: (" 
              << spawnPosition.x << ", " << spawnPosition.y << ", " << spawnPosition.z << ")" << std::endl;
}

void ArrowSystem::StopShooting() {
    isShooting = false;
}

