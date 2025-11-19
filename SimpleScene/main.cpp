#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "shader.h"
#include "camera.h"
// #define STB_IMAGE_IMPLEMENTATION
#include "model.h"
#include "sandbox_mesh.h"
#include "particle_system.h"
#include "stb_image.h"

#include <iostream>
#include <vector>
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void processInput(GLFWwindow* window);
// 生成窗户样式的顶点
std::vector<float> generateCircularWindowVertices(int segments, float outerRadius, float innerRadius, float barWidth, float depth);
// 生成带洞的墙壁
std::vector<float> generateWallWithHoleVertices(float width, float height, float holeRadius, int segments);

// 屏幕设置
const unsigned int SCR_WIDTH = 1600;
const unsigned int SCR_HEIGHT = 1200;

// 相机
Camera camera(glm::vec3(0.0f, 0.0f, 5.0f));
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;

// 雨云状态
bool isCloudVisible = false;
glm::vec3 cloudPositionOffset(0.0f, 1.0f, 0.0f);    // 云相对于沙盘中心的偏移
bool mKeyPressed = false;                           // M键状态
bool isRaining = false;                             // 下雨状态
bool rKeyPressed = false;                           // R键状态
bool isSnowing = false;                             // 下雪状态
bool bKeyPressed = false;                           // B键状态

// 时间管理
float deltaTime = 0.0f;
float lastFrame = 0.0f;

// 灯光位置
glm::vec3 lightPos(0.0f, 2.5f, 3.0f);

int main()
{
    // 初始化并配置glfw
    // ------------------------------
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    // glfw窗口创建
    // --------------------
    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "LearnOpenGL", NULL, NULL);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);

    // 告诉 GLFW 我们要捕获鼠标
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // glad: 加载所有 OpenGL 函数指针
    // ---------------------------------------
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }
    stbi_set_flip_vertically_on_load(true);

    // 配置全局 OpenGL 状态
    // -----------------------------
    glEnable(GL_DEPTH_TEST);

    // Shader
    // ------------------------------------
    Shader lightingShader("lighting.vs", "lighting.fs");
    Shader lightCubeShader("lightcube.vs", "lightcube.fs");
    Shader modelShader("model_loading.vs", "model_loading.fs");
    Shader sandboxShader("sandbox.vs", "sandbox.fs");
    Shader cloudShader("cloud.vs", "cloud.fs");

    // --- 加载模型 ---
    Model ourModel("resource/model/table3.obj");
    Model lampModel("resource/lamp/lamp1.obj");

    // --- 创建沙盘实例 ---
    // 方法1：从高度图创建
    TerrainSandbox sandbox_heightmap(
        2.0f, // 宽度
        1.8f, // 深度
        128,  // 网格精度
        TerrainSandbox::GenMethod::HEIGHTMAP,
        "resource/textures/heightmap1.png", // 高度图路径
        "resource/textures/red_sand_diff_4k.jpg",  // 沙子纹理路径
        "resource/textures/grass.jpg", // 草地纹理路径
        "resource/textures/snow2.png", // 雪地纹理路径
        "resource/textures/red_sand_disp_4k.png" // 法线/灰度图路径
    );

    // // 方法2：程序化随机生成
    // TerrainSandbox sandbox_procedural(
    //     1.5f, 1.0f, 64,
    //     TerrainSandbox::GenMethod::PROCEDURAL_RANDOM,
    //     nullptr,
    //     "resource/textures/red_sand_diff_4k.jpg"
    // );

    // --- 初始化粒子系统 ---
    ParticleSystem rainSystem(2000, &sandbox_heightmap);
    ParticleSystem snowSystem(3000, &sandbox_heightmap, "snow.vs", "snow.fs");

    // --- 加载云纹理 ---
    unsigned int cloudTexture;
    glGenTextures(1, &cloudTexture);
    glBindTexture(GL_TEXTURE_2D, cloudTexture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    int width, height, nrChannels;
    unsigned char *data = stbi_load("resource/textures/cloud.png", &width, &height, &nrChannels, 0);
    if (data)
    {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
    }
    else
    {
        std::cout << "Failed to load cloud texture" << std::endl;
    }
    stbi_image_free(data);

    // --- 创建云的平面顶点 ---
    float cloudVertices[] = {
        // positions         // texture Coords
        -1.0f, 0.0f, -0.5f,  0.0f, 1.0f,
         1.0f, 0.0f, -0.5f,  1.0f, 1.0f,
         1.0f, 0.0f,  0.5f,  1.0f, 0.0f,
        -1.0f, 0.0f,  0.5f,  0.0f, 0.0f
    };
    unsigned int cloudIndices[] = {
        0, 1, 2,
        2, 3, 0
    };
    unsigned int cloudVAO, cloudVBO, cloudEBO;
    glGenVertexArrays(1, &cloudVAO);
    glGenBuffers(1, &cloudVBO);
    glGenBuffers(1, &cloudEBO);
    glBindVertexArray(cloudVAO);
    glBindBuffer(GL_ARRAY_BUFFER, cloudVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(cloudVertices), cloudVertices, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, cloudEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(cloudIndices), cloudIndices, GL_STATIC_DRAW);
    // Position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    // Texture coord attribute
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glBindVertexArray(0);

    // 统一房间用的顶点信息(每一个前面三个值为顶点坐标，后面三个值为法线向量)
    // ------------------------------------------------------------------
    float vertices[] = {
        // positions          // normals 
        -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
         0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
         0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
         0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
        -0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
        -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,

        -0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,
         0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,
        -0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,
        -0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,

        -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,
        -0.5f,  0.5f, -0.5f, -1.0f,  0.0f,  0.0f,
        -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,
        -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,
        -0.5f, -0.5f,  0.5f, -1.0f,  0.0f,  0.0f,
        -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,

         0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,
         0.5f,  0.5f, -0.5f,  1.0f,  0.0f,  0.0f,
         0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,
         0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,
         0.5f, -0.5f,  0.5f,  1.0f,  0.0f,  0.0f,
         0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,

        -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,
         0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,
         0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,
         0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,
        -0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,
        -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,

        -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,
         0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,
        -0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,
        -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f
    };

    // --- 创建圆形窗户的顶点数据 ---
    // ------------------------------------------------------------------
    float windowOuterRadius = 1.2f;
    float windowScale = 1.2f;
    // std::vector<float> circularWindowVertices = generateCircularWindowVertices(72, windowOuterRadius, 1.1f, 0.05f);
    std::vector<float> circularWindowVertices = generateCircularWindowVertices(72, windowOuterRadius, 1.1f, 0.05f, 0.1f);    
    int windowVertexCount = circularWindowVertices.size() / 6;

    // --- 生成带洞的墙壁的顶点数据 ---
    std::vector<float> wallWithHoleVertices = generateWallWithHoleVertices(8.0f, 6.0f, windowOuterRadius * windowScale, 72);
    int wallWithHoleVertexCount = wallWithHoleVertices.size() / 6;

    // 创建墙壁、地板、天花板的VAO/VBO
    // ------------------------------------------------------------------
    unsigned int roomVAO, roomVBO;
    glGenVertexArrays(1, &roomVAO);
    glGenBuffers(1, &roomVBO);
    glBindVertexArray(roomVAO);
    glBindBuffer(GL_ARRAY_BUFFER, roomVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    // 位置属性
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    // 法线属性
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    // 创建窗户VAO/VBO
    // ------------------------------------------------------------------
    unsigned int windowVAO, windowVBO;
    glGenVertexArrays(1, &windowVAO);
    glGenBuffers(1, &windowVBO);
    glBindVertexArray(windowVAO);
    glBindBuffer(GL_ARRAY_BUFFER, windowVBO);
    glBufferData(GL_ARRAY_BUFFER, circularWindowVertices.size() * sizeof(float), circularWindowVertices.data(), GL_STATIC_DRAW);
    // 位置属性
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    // 法线属性
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    // 光源方块的顶点信息
    // ------------------------------------------------------------------
    unsigned int lightCubeVAO;
    glGenVertexArrays(1, &lightCubeVAO);
    glBindVertexArray(lightCubeVAO);
    // 只绑定VBO, 因为它的数据和房间的顶点一样
    glBindBuffer(GL_ARRAY_BUFFER, roomVBO);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    
    // 创建后墙的VAO/VBO
    // ------------------------------------------------------------------
    unsigned int wallVAO, wallVBO;
    glGenVertexArrays(1, &wallVAO);
    glGenBuffers(1, &wallVBO);
    glBindVertexArray(wallVAO);
    glBindBuffer(GL_ARRAY_BUFFER, wallVBO);
    glBufferData(GL_ARRAY_BUFFER, wallWithHoleVertices.size() * sizeof(float), wallWithHoleVertices.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    // 渲染循环
    // -----------
    while (!glfwWindowShouldClose(window))
    {
        // 时间逻辑
        // --------------------
        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // 输入
        // -----
        processInput(window);

        // 开始渲染
        // ------
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // --- 定义场景原点，将所有物体移动到房子中心 ---
        glm::vec3 sceneOrigin = glm::vec3(0.0f, 0.0f, 0.0f); 

        // 将光源移动到房间正上方
        glm::vec3 lightPos = glm::vec3(0.0f, 2.5f, 0.0f);


        // 整体光照强度
        glm::vec3 warmColor(1.0f, 0.85f, 0.6f);
        float overallIntensity = 1.0f; // 整体亮度
        glm::vec3 finalLightColor = warmColor * overallIntensity;

        // 确保在设置 Uniforms/Drawing 之前激活 Shader
        //---------------------------------------------------------------------
        lightingShader.use();
        lightingShader.setVec3("lightPos", lightPos); // <--- 使用新的光源位置
        lightingShader.setVec3("viewPos", camera.Position);
        lightingShader.setVec3("lightColor", finalLightColor);

        glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
        glm::mat4 view = camera.GetViewMatrix();
        lightingShader.setMat4("projection", projection);
        lightingShader.setMat4("view", view);

        glm::mat4 model = glm::mat4(1.0f);

        glBindVertexArray(roomVAO);

        // 绘制天花板
        {
            //设置物体颜色
            lightingShader.setVec3("objectColor", 0.5, 0.5f, 0.5f);

            // 设置模型变换
            model = glm::mat4(1.0f);
            model = glm::translate(model, glm::vec3(0.0f, 3.0f, 0.0f));
            model = glm::scale(model, glm::vec3(8.0f, 0.1f, 8.0f));
            lightingShader.setMat4("model", model);

            // 渲染
            glDrawArrays(GL_TRIANGLES, 0, 36);
        }

        // 绘制地板
        {
            //设置物体颜色
            lightingShader.use();
            lightingShader.setVec3("objectColor", 0.4f, 0.3f, 0.25f);

            // 设置模型变换
            model = glm::mat4(1.0f);
            model = glm::translate(model, glm::vec3(0.0f, -3.0f, 0.0f));
            model = glm::scale(model, glm::vec3(8.0f, 0.1f, 8.0f));
            lightingShader.setMat4("model", model);

            // 渲染
            glDrawArrays(GL_TRIANGLES, 0, 36);
        }

        // 绘制左墙
        {
            //设置物体颜色
            lightingShader.use();
            lightingShader.setVec3("objectColor", 0.9f, 0.85f, 0.7f);

            // 设置模型变换
            model = glm::mat4(1.0f);
            model = glm::translate(model, glm::vec3(-4.0f, 0.0f, 0.0f));
            model = glm::scale(model, glm::vec3(0.1f, 6.0f, 8.0f));
            lightingShader.setMat4("model", model);

            // 渲染
            glDrawArrays(GL_TRIANGLES, 0, 36);
        }

        // 绘制右墙
        {
            //设置物体颜色
            lightingShader.use();
            lightingShader.setVec3("objectColor", 0.9f, 0.85f, 0.7f);

            // 设置模型变换
            model = glm::mat4(1.0f);
            model = glm::translate(model, glm::vec3(4.0f, 0.0f, 0.0f));
            model = glm::scale(model, glm::vec3(0.1f, 6.0f, 8.0f));
            lightingShader.setMat4("model", model);

            // 渲染
            glDrawArrays(GL_TRIANGLES, 0, 36);
        }

        // // 绘制后墙
        // {
        //     //设置物体颜色
        //     lightingShader.setVec3("objectColor", 1.0f, 1.0f, 1.0f);

        //     // 设置模型变换
        //     model = glm::mat4(1.0f);
        //     model = glm::translate(model, glm::vec3(0.0f, 0.0f, -4.0f));
        //     model = glm::scale(model, glm::vec3(8.0f, 6.0f, 0.1f));
        //     lightingShader.setMat4("model", model);

        //     // 渲染
        //     glDrawArrays(GL_TRIANGLES, 0, 36);
        // }
        
        // 绘制带洞的后墙
        {
            //设置物体颜色
            lightingShader.use();
            lightingShader.setVec3("objectColor", 1.0f, 1.0f, 1.0f);

            // 设置模型变换
            model = glm::mat4(1.0f);
            model = glm::translate(model, glm::vec3(0.0f, 0.0f, -4.0f));
            // 不需要 scale，因为顶点已经定义了正确的尺寸
            lightingShader.setMat4("model", model);

            // 渲染带洞的墙
            glBindVertexArray(wallVAO);
            glDrawArrays(GL_TRIANGLES, 0, wallWithHoleVertexCount);
        }

        // 绘制窗户
        {
            //设置物体颜色
            lightingShader.use();
            lightingShader.setVec3("objectColor", 0.36, 0.2f, 0.09f); // 木头颜色

            // 设置模型变换
            model = glm::mat4(1.0f);
            model = glm::translate(model, glm::vec3(0.0f, 0.0f, -4.0f)); // 放在后墙前一点
            model = glm::scale(model, glm::vec3(1.2f));
            lightingShader.setMat4("model", model);
            glBindVertexArray(windowVAO);
            glDrawArrays(GL_TRIANGLES, 0, windowVertexCount); 
        }

        // 绘制灯
        {
            lightCubeShader.use();
            lightCubeShader.setMat4("projection", projection);
            lightCubeShader.setMat4("view", view);
            model = glm::mat4(1.0f);
            model = glm::translate(model, lightPos);
            model = glm::scale(model, glm::vec3(0.2f)); // a smaller cube
            lightCubeShader.setMat4("model", model);

            glBindVertexArray(lightCubeVAO);
            glDrawArrays(GL_TRIANGLES, 0, 36);
        }

        // --- 绘制书桌模型 ---
        {
            modelShader.use();
            modelShader.setMat4("projection", projection);
            modelShader.setMat4("view", view);

            // 设置光照
            modelShader.setVec3("viewPos", camera.Position);
            modelShader.setVec3("light.position", lightPos); // <--- 使用新的光源位置
            modelShader.setVec3("light.ambient", finalLightColor * 0.2f);
            modelShader.setVec3("light.diffuse", finalLightColor);
            modelShader.setVec3("light.specular", finalLightColor * 0.2f);
        
            // 渲染桌子的模型
            model = glm::mat4(1.0f);
            model = glm::translate(model, sceneOrigin + glm::vec3(0.0f, -3.0f, 0.0f));
            model = glm::scale(model, glm::vec3(0.5f, 0.5f, 0.7f));	// 书桌的模型
            model = glm::rotate(model, glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f)); // 旋转90度，使桌子朝前
            modelShader.setMat4("model", model);
            ourModel.Draw(modelShader);
        }

        // --- 绘制台灯模型 ---
        {
            // modelShader 已经激活，且 view/projection/light 等 uniform 已设置
            // 所以只需要为台灯设置一个新的 model 矩阵
            modelShader.use(); 
            model = glm::mat4(1.0f);
            // 把台灯移动到桌子上的一个偏左位置
            glm::vec3 lampModelWorldPos = sceneOrigin + glm::vec3(-1.0f, -0.8f, -0.5f);
            model = glm::translate(model, lampModelWorldPos); // 使用计算好的世界坐标
            model = glm::scale(model, glm::vec3(0.3f)); // 调整台灯使尺寸合适
            modelShader.setMat4("model", model);
            lampModel.Draw(modelShader);
            
        }

        // --- 绘制地形沙盘 ---
        {
            sandboxShader.use();
            sandboxShader.setMat4("projection", projection);
            sandboxShader.setMat4("view", view);
            sandboxShader.setVec3("viewPos", camera.Position);
            sandboxShader.setVec3("lightPos", lightPos); // <--- 使用新的光源位置
            sandboxShader.setVec3("lightColor", finalLightColor);

            model = glm::mat4(1.0f);
            // 将沙盘放在书桌上
            model = glm::translate(model, sceneOrigin + glm::vec3(0.5f, -1.4f, -1.0f)); 
            sandboxShader.setMat4("model", model);

            // 绘制沙盘 (根据选择的方法)
            sandbox_heightmap.Draw(sandboxShader);
            // sandbox_procedural.Draw(sandboxShader);
        }

        // --- 绘制雨云 (如果可见) ---
        // 沙盘的世界位置应该只在这里定义一次
        glm::vec3 sandboxWorldPos = sceneOrigin + glm::vec3(0.5f, -1.4f, -1.0f);
        // 云的世界中心位置，Y值基于沙盘和云的偏移量
        glm::vec3 cloudWorldCenter = sandboxWorldPos + cloudPositionOffset;

        if (isCloudVisible)
        {
            // 启用混合以支持透明度
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

            cloudShader.use();
            cloudShader.setMat4("projection", projection);
            cloudShader.setMat4("view", view);

            // 绑定云纹理 (只需要绑定一次)
            glActiveTexture(GL_TEXTURE0);
            cloudShader.setInt("cloudTexture", 0);
            glBindTexture(GL_TEXTURE_2D, cloudTexture);

            // --- 使用分层渲染来增加云的厚度 ---
            const int numCloudLayers = 10; // 定义云的层数
            const float layerSpacing = 0.01f; // 定义每层之间的间距

            // 计算云的基础位置和缩放
            glm::vec3 sandboxBasePos = sceneOrigin + glm::vec3(0.5f, -1.4f, -0.5f);
            glm::mat4 baseModel = glm::mat4(1.0f);
            baseModel = glm::translate(baseModel, sandboxWorldPos + cloudPositionOffset);
            baseModel = glm::scale(baseModel, glm::vec3(1.0f)); // 调整云的大小
            // baseModel = glm::rotate(baseModel, glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));

            glBindVertexArray(cloudVAO);

            for (int i = 0; i < numCloudLayers; ++i)
            {
                // 为当前层计算模型矩阵
                glm::mat4 layerModel = glm::translate(baseModel, glm::vec3(0.0f, i * layerSpacing, 0.0f));
                cloudShader.setMat4("model", layerModel);

                // 为当前层计算纹理偏移，制造视差效果
                // 这里的偏移量可以根据喜好调整
                float offsetFactor = static_cast<float>(i) / static_cast<float>(numCloudLayers);
                cloudShader.setVec2("texOffset", glm::vec2(offsetFactor * 0.1f, offsetFactor * 0.1f));

                // 绘制当前层
                glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
            }

            glBindVertexArray(0);

            // 绘制完毕后禁用混合，以免影响其他物体
            glDisable(GL_BLEND);

            // --- 更新和绘制粒子 ---
            if (isRaining)
            {
                rainSystem.Update(deltaTime, cloudWorldCenter, sandboxWorldPos, ParticleSystem::EffectType::GROWTH);
                rainSystem.Draw(view, projection);
            }
            if (isSnowing)
            {
                snowSystem.Update(deltaTime, cloudWorldCenter, sandboxWorldPos, ParticleSystem::EffectType::SNOW);
                snowSystem.Draw(view, projection, true); // true 表示以点的形式绘制
            }
        }



        // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
        // -------------------------------------------------------------------------------
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // optional: de-allocate all resources once they've outlived their purpose:
    // ------------------------------------------------------------------------
    glDeleteVertexArrays(1, &roomVAO);
    glDeleteVertexArrays(1, &lightCubeVAO);
    glDeleteVertexArrays(1, &windowVAO);
    glDeleteVertexArrays(1, &wallVAO);
    glDeleteVertexArrays(1, &cloudVAO); // <-- 清理云VAO
    glDeleteBuffers(1, &roomVBO);
    glDeleteBuffers(1, &windowVBO);
    glDeleteBuffers(1, &wallVBO);
    glDeleteBuffers(1, &cloudVBO); // <-- 清理云VBO
    glDeleteBuffers(1, &cloudEBO); // <-- 清理云EBO

    // glfw: terminate, clearing all previously allocated GLFW resources.
    // ------------------------------------------------------------------
    glfwTerminate();
    return 0;
}

// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
// ---------------------------------------------------------------------------------------------------------
void processInput(GLFWwindow* window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    // 相机移动
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        camera.ProcessKeyboard(FORWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        camera.ProcessKeyboard(BACKWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        camera.ProcessKeyboard(LEFT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        camera.ProcessKeyboard(RIGHT, deltaTime);

    // 雨云控制
    // 切换雨云可见性
    if (glfwGetKey(window, GLFW_KEY_M) == GLFW_PRESS && !mKeyPressed)
    {
        isCloudVisible = !isCloudVisible;
        mKeyPressed = true;
    }
    if (glfwGetKey(window, GLFW_KEY_M) == GLFW_RELEASE)
    {
        mKeyPressed = false;
    }

    if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS && !rKeyPressed)
    {
        if (isCloudVisible) { // 只有云可见时才能下雨
            isRaining = !isRaining;
            if (isRaining) isSnowing = false; // 下雨时停止下雪
        }
        rKeyPressed = true;
    }

    if (glfwGetKey(window, GLFW_KEY_R) == GLFW_RELEASE)
    {
        rKeyPressed = false;
    }

    if (glfwGetKey(window, GLFW_KEY_B) == GLFW_PRESS && !bKeyPressed)
    {
        if (isCloudVisible) { // 只有云可见时才能下雪
            isSnowing = !isSnowing;
            if (isSnowing) isRaining = false; // 下雪时停止下雨
        }
        bKeyPressed = true;
    }

    if (glfwGetKey(window, GLFW_KEY_B) == GLFW_RELEASE)
    {
        bKeyPressed = false;
    }


    // 移动雨云
    if (isCloudVisible)
    {
        float cloudSpeed = 1.0f * deltaTime;
        if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS)
            cloudPositionOffset.z -= cloudSpeed;
        if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS)
            cloudPositionOffset.z += cloudSpeed;
        if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS)
            cloudPositionOffset.x -= cloudSpeed;
        if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS)
            cloudPositionOffset.x += cloudSpeed;
    }

}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    // make sure the viewport matches the new window dimensions; note that width and 
    // height will be significantly larger than specified on retina displays.
    glViewport(0, 0, width, height);
}

// glfw: whenever the mouse moves, this callback is called
// -------------------------------------------------------
void mouse_callback(GLFWwindow* window, double xposIn, double yposIn)
{
    float xpos = static_cast<float>(xposIn);
    float ypos = static_cast<float>(yposIn);
    if (firstMouse)
    {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos; // reversed since y-coordinates go from bottom to top

    lastX = xpos;
    lastY = ypos;

    camera.ProcessMouseMovement(xoffset, yoffset);
}

// glfw: whenever the mouse scroll wheel scrolls, this callback is called
// ----------------------------------------------------------------------
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    camera.ProcessMouseScroll(static_cast<float>(yoffset));
}

// 将生成圆形窗户的逻辑封装到一个独立的函数
std::vector<float> generateCircularWindowVertices(int segments, float outerRadius, float innerRadius, float barWidth, float depth)
{
    std::vector<float> vertices;
    const float twoPI = 2.0f * static_cast<float>(M_PI);
    float halfDepth = depth / 2.0f;

    // 辅助函数，用于添加一个3D条带 (用于圆环部分)
    auto add3DStrip = [&](glm::vec2 p1_inner, glm::vec2 p1_outer, glm::vec2 p2_inner, glm::vec2 p2_outer) {
        // Front face
        vertices.insert(vertices.end(), { p1_outer.x, p1_outer.y, halfDepth, 0, 0, 1 });
        vertices.insert(vertices.end(), { p1_inner.x, p1_inner.y, halfDepth, 0, 0, 1 });
        vertices.insert(vertices.end(), { p2_inner.x, p2_inner.y, halfDepth, 0, 0, 1 });
        vertices.insert(vertices.end(), { p1_outer.x, p1_outer.y, halfDepth, 0, 0, 1 });
        vertices.insert(vertices.end(), { p2_inner.x, p2_inner.y, halfDepth, 0, 0, 1 });
        vertices.insert(vertices.end(), { p2_outer.x, p2_outer.y, halfDepth, 0, 0, 1 });

        // Back face (flipped winding order)
        vertices.insert(vertices.end(), { p1_outer.x, p1_outer.y, -halfDepth, 0, 0, -1 });
        vertices.insert(vertices.end(), { p2_inner.x, p2_inner.y, -halfDepth, 0, 0, -1 });
        vertices.insert(vertices.end(), { p1_inner.x, p1_inner.y, -halfDepth, 0, 0, -1 });
        vertices.insert(vertices.end(), { p1_outer.x, p1_outer.y, -halfDepth, 0, 0, -1 });
        vertices.insert(vertices.end(), { p2_outer.x, p2_outer.y, -halfDepth, 0, 0, -1 });
        vertices.insert(vertices.end(), { p2_inner.x, p2_inner.y, -halfDepth, 0, 0, -1 });

        // Outer side
        glm::vec3 outer_normal = glm::normalize(glm::vec3(p1_outer.x + p2_outer.x, p1_outer.y + p2_outer.y, 0));
        vertices.insert(vertices.end(), { p1_outer.x, p1_outer.y, halfDepth, outer_normal.x, outer_normal.y, 0 });
        vertices.insert(vertices.end(), { p1_outer.x, p1_outer.y, -halfDepth, outer_normal.x, outer_normal.y, 0 });
        vertices.insert(vertices.end(), { p2_outer.x, p2_outer.y, -halfDepth, outer_normal.x, outer_normal.y, 0 });
        vertices.insert(vertices.end(), { p1_outer.x, p1_outer.y, halfDepth, outer_normal.x, outer_normal.y, 0 });
        vertices.insert(vertices.end(), { p2_outer.x, p2_outer.y, -halfDepth, outer_normal.x, outer_normal.y, 0 });
        vertices.insert(vertices.end(), { p2_outer.x, p2_outer.y, halfDepth, outer_normal.x, outer_normal.y, 0 });

        // Inner side
        glm::vec3 inner_normal = -glm::normalize(glm::vec3(p1_inner.x + p2_inner.x, p1_inner.y + p2_inner.y, 0));
        vertices.insert(vertices.end(), { p1_inner.x, p1_inner.y, halfDepth, inner_normal.x, inner_normal.y, 0 });
        vertices.insert(vertices.end(), { p2_inner.x, p2_inner.y, -halfDepth, inner_normal.x, inner_normal.y, 0 });
        vertices.insert(vertices.end(), { p1_inner.x, p1_inner.y, -halfDepth, inner_normal.x, inner_normal.y, 0 });
        vertices.insert(vertices.end(), { p1_inner.x, p1_inner.y, halfDepth, inner_normal.x, inner_normal.y, 0 });
        vertices.insert(vertices.end(), { p2_inner.x, p2_inner.y, halfDepth, inner_normal.x, inner_normal.y, 0 });
        vertices.insert(vertices.end(), { p2_inner.x, p2_inner.y, -halfDepth, inner_normal.x, inner_normal.y, 0 });
    };
    
    // 辅助函数，用于添加一个3D矩形 (用于直线部分)
    auto add3DRect = [&](float x1, float y1, float x2, float y2) {
        // Front
        vertices.insert(vertices.end(), { x1, y1, halfDepth, 0, 0, 1 });
        vertices.insert(vertices.end(), { x2, y1, halfDepth, 0, 0, 1 });
        vertices.insert(vertices.end(), { x2, y2, halfDepth, 0, 0, 1 });
        vertices.insert(vertices.end(), { x1, y1, halfDepth, 0, 0, 1 });
        vertices.insert(vertices.end(), { x2, y2, halfDepth, 0, 0, 1 });
        vertices.insert(vertices.end(), { x1, y2, halfDepth, 0, 0, 1 });
        // Back
        vertices.insert(vertices.end(), { x1, y1, -halfDepth, 0, 0, -1 });
        vertices.insert(vertices.end(), { x2, y2, -halfDepth, 0, 0, -1 });
        vertices.insert(vertices.end(), { x2, y1, -halfDepth, 0, 0, -1 });
        vertices.insert(vertices.end(), { x1, y1, -halfDepth, 0, 0, -1 });
        vertices.insert(vertices.end(), { x1, y2, -halfDepth, 0, 0, -1 });
        vertices.insert(vertices.end(), { x2, y2, -halfDepth, 0, 0, -1 });
        // Top
        vertices.insert(vertices.end(), { x1, y2, halfDepth, 0, 1, 0 });
        vertices.insert(vertices.end(), { x2, y2, halfDepth, 0, 1, 0 });
        vertices.insert(vertices.end(), { x2, y2, -halfDepth, 0, 1, 0 });
        vertices.insert(vertices.end(), { x1, y2, halfDepth, 0, 1, 0 });
        vertices.insert(vertices.end(), { x2, y2, -halfDepth, 0, 1, 0 });
        vertices.insert(vertices.end(), { x1, y2, -halfDepth, 0, 1, 0 });
        // Bottom
        vertices.insert(vertices.end(), { x1, y1, halfDepth, 0, -1, 0 });
        vertices.insert(vertices.end(), { x2, y1, -halfDepth, 0, -1, 0 });
        vertices.insert(vertices.end(), { x2, y1, halfDepth, 0, -1, 0 });
        vertices.insert(vertices.end(), { x1, y1, halfDepth, 0, -1, 0 });
        vertices.insert(vertices.end(), { x1, y1, -halfDepth, 0, -1, 0 });
        vertices.insert(vertices.end(), { x2, y1, -halfDepth, 0, -1, 0 });
        // Left
        vertices.insert(vertices.end(), { x1, y1, halfDepth, -1, 0, 0 });
        vertices.insert(vertices.end(), { x1, y2, -halfDepth, -1, 0, 0 });
        vertices.insert(vertices.end(), { x1, y1, -halfDepth, -1, 0, 0 });
        vertices.insert(vertices.end(), { x1, y1, halfDepth, -1, 0, 0 });
        vertices.insert(vertices.end(), { x1, y2, halfDepth, -1, 0, 0 });
        vertices.insert(vertices.end(), { x1, y2, -halfDepth, -1, 0, 0 });
        // Right
        vertices.insert(vertices.end(), { x2, y1, halfDepth, 1, 0, 0 });
        vertices.insert(vertices.end(), { x2, y1, -halfDepth, 1, 0, 0 });
        vertices.insert(vertices.end(), { x2, y2, -halfDepth, 1, 0, 0 });
        vertices.insert(vertices.end(), { x2, y1, halfDepth, 1, 0, 0 });
        vertices.insert(vertices.end(), { x2, y2, -halfDepth, 1, 0, 0 });
        vertices.insert(vertices.end(), { x2, y2, halfDepth, 1, 0, 0 });
    };

    // 1. 外圆环 (annulus)
    for (int i = 0; i < segments; ++i) {
        float angle1 = twoPI * static_cast<float>(i) / static_cast<float>(segments);
        float angle2 = twoPI * static_cast<float>(i + 1) / static_cast<float>(segments);
        
        glm::vec2 p1_outer(outerRadius * cosf(angle1), outerRadius * sinf(angle1));
        glm::vec2 p2_outer(outerRadius * cosf(angle2), outerRadius * sinf(angle2));
        glm::vec2 p1_inner(innerRadius * cosf(angle1), innerRadius * sinf(angle1));
        glm::vec2 p2_inner(innerRadius * cosf(angle2), innerRadius * sinf(angle2));
        
        add3DStrip(p1_inner, p1_outer, p2_inner, p2_outer);
    }

    // 2. 内部十字交叉
    add3DRect(-innerRadius, -barWidth, innerRadius, barWidth); // 水平条
    add3DRect(-barWidth, -innerRadius, barWidth, innerRadius); // 垂直条

    // 3. 四个角落的装饰性弧形
    const int patternSegments = 20;
    const float patternThickness = 0.034f;
    const float centerDist = 0.15f;
    const float arcRadius = centerDist;

    for (int q = 0; q < 4; ++q) {
        glm::vec2 center(0.0f, 0.0f);
        float startAngle = 0.0f;

        if (q == 0) { center = glm::vec2(centerDist, 0.0f); startAngle = static_cast<float>(M_PI) / 2.0f; }
        else if (q == 1) { center = glm::vec2(0.0f, centerDist); startAngle = static_cast<float>(M_PI); }
        else if (q == 2) { center = glm::vec2(-centerDist, 0.0f); startAngle = -static_cast<float>(M_PI) / 2.0f; }
        else { center = glm::vec2(0.0f, -centerDist); startAngle = 0.0f; }

        for (int i = -1; i < patternSegments + 1; ++i) {
            float angle1 = startAngle - (static_cast<float>(i) / patternSegments) * static_cast<float>(M_PI);
            float angle2 = startAngle - (static_cast<float>(i + 1) / patternSegments) * static_cast<float>(M_PI);

            glm::vec2 p1_center = center + glm::vec2(arcRadius * cosf(angle1), arcRadius * sinf(angle1));
            glm::vec2 p2_center = center + glm::vec2(arcRadius * cosf(angle2), arcRadius * sinf(angle2));
            glm::vec2 normal1 = glm::normalize(p1_center - center);
            glm::vec2 normal2 = glm::normalize(p2_center - center);
            glm::vec2 p1_outer = p1_center + normal1 * patternThickness * 0.5f;
            glm::vec2 p1_inner = p1_center - normal1 * patternThickness * 0.5f;
            glm::vec2 p2_outer = p2_center + normal2 * patternThickness * 0.5f;
            glm::vec2 p2_inner = p2_center - normal2 * patternThickness * 0.5f;
            
            add3DStrip(p1_inner, p1_outer, p2_inner, p2_outer);
        }
    }
    
    // 4.内部装饰性结构 四个小正方形
    // 上
    add3DRect(-0.2f * innerRadius, 0.4f * innerRadius - barWidth, 0.2f * innerRadius, 0.4f * innerRadius + barWidth);
    add3DRect(-0.2f * innerRadius, 0.8f * innerRadius - barWidth, 0.2f * innerRadius, 0.8f * innerRadius);
    add3DRect(0.2f * innerRadius - barWidth, 0.4f * innerRadius, 0.2f * innerRadius, 0.8f * innerRadius);
    add3DRect(-0.2f * innerRadius, 0.4f * innerRadius, -0.2f * innerRadius + barWidth, 0.8f * innerRadius);
    // 下
    add3DRect(-0.2f * innerRadius, -0.4f * innerRadius - barWidth, 0.2f * innerRadius, -0.4f * innerRadius + barWidth);
    add3DRect(-0.2f * innerRadius, -0.8f * innerRadius - barWidth, 0.2f * innerRadius, -0.8f * innerRadius);
    add3DRect(0.2f * innerRadius - barWidth, -0.8f * innerRadius, 0.2f * innerRadius, -0.4f * innerRadius);
    add3DRect(-0.2f * innerRadius, -0.8f * innerRadius, -0.2f * innerRadius + barWidth, -0.4f * innerRadius);
    // 右
    add3DRect(0.4f * innerRadius - barWidth, -0.2f * innerRadius, 0.4f * innerRadius + barWidth, 0.2f * innerRadius);
    add3DRect(0.8f * innerRadius - barWidth, -0.2f * innerRadius, 0.8f * innerRadius, 0.2f * innerRadius);
    add3DRect(0.4f * innerRadius, 0.2f * innerRadius - barWidth, 0.8f * innerRadius, 0.2f * innerRadius);
    add3DRect(0.4f * innerRadius, -0.2f * innerRadius, 0.8f * innerRadius, -0.2f * innerRadius + barWidth);
    // 左
    add3DRect(-0.4f * innerRadius - barWidth, -0.2f * innerRadius, -0.4f * innerRadius + barWidth, 0.2f * innerRadius);
    add3DRect(-0.8f * innerRadius - barWidth, -0.2f * innerRadius, -0.8f * innerRadius, 0.2f * innerRadius);
    add3DRect(-0.8f * innerRadius, 0.2f * innerRadius - barWidth, -0.4f * innerRadius, 0.2f * innerRadius);
    add3DRect(-0.8f * innerRadius, -0.2f * innerRadius, -0.4f * innerRadius, -0.2f * innerRadius + barWidth);

    // 添加斜线
    // (为简化，斜线用一个很平的矩形表示，而不是精确的斜边)
    add3DRect(0.2f * innerRadius, 0.4f * innerRadius - barWidth, 0.4f * innerRadius + barWidth, 0.2f * innerRadius);
    add3DRect(-0.4f * innerRadius - barWidth, 0.2f * innerRadius, -0.2f * innerRadius, 0.4f * innerRadius + barWidth);
    add3DRect(-0.4f * innerRadius - barWidth, -0.2f * innerRadius, -0.2f * innerRadius, -0.4f * innerRadius - barWidth);
    add3DRect(0.2f * innerRadius, -0.4f * innerRadius - barWidth, 0.4f * innerRadius + barWidth, -0.2f * innerRadius);

    // 在每个象限的中间画一条水平和一条垂直线
    add3DRect(-0.85f * innerRadius, 0.6f * innerRadius - 0.5f * barWidth, 0.85f * innerRadius, 0.6f * innerRadius + 0.5f * barWidth); // 上
    add3DRect(-0.85f * innerRadius, -0.6f * innerRadius - 0.5f * barWidth, 0.85f * innerRadius, -0.6f * innerRadius + 0.5f * barWidth); // 下
    add3DRect(-0.6f * innerRadius - 0.5f * barWidth, -0.85f * innerRadius, -0.6f * innerRadius + 0.5f * barWidth, 0.85f * innerRadius); // 左
    add3DRect(0.6f * innerRadius - 0.5f * barWidth, -0.85f * innerRadius, 0.6f * innerRadius + 0.5f * barWidth, 0.85f * innerRadius); // 右

    // 从每条斜线的中间点开始，向外画一条直线
    // (为简化，同样用较平的矩形表示)
    add3DRect(-0.3f * innerRadius - 0.5f * barWidth, 0.3f * innerRadius + 0.5f * barWidth, -0.3f * innerRadius + 0.5f * barWidth, 0.96f * innerRadius); // 左上
    add3DRect(0.3f * innerRadius - 0.5f * barWidth, 0.3f * innerRadius + 0.5f * barWidth, 0.3f * innerRadius + 0.5f * barWidth, 0.96f * innerRadius); // 右上
    add3DRect(-0.3f * innerRadius - 0.5f * barWidth, -0.96f * innerRadius, -0.3f * innerRadius + 0.5f * barWidth, -0.3f * innerRadius - 0.5f * barWidth); // 左下
    add3DRect(0.3f * innerRadius - 0.5f * barWidth, -0.96f * innerRadius, 0.3f * innerRadius + 0.5f * barWidth, -0.3f * innerRadius - 0.5f * barWidth); // 右下
    add3DRect(0.3f * innerRadius + 0.5f * barWidth, 0.3f * innerRadius - 0.5f * barWidth, 0.96f * innerRadius, 0.3f * innerRadius + 0.5f * barWidth); // 右上
    add3DRect(0.3f * innerRadius + 0.5f * barWidth, -0.3f * innerRadius - 0.5f * barWidth, 0.96f * innerRadius, -0.3f * innerRadius + 0.5f * barWidth); // 右下
    add3DRect(-0.96f * innerRadius, 0.3f * innerRadius - 0.5f * barWidth, -0.3f * innerRadius - 0.5f * barWidth, 0.3f * innerRadius + 0.5f * barWidth); // 左上
    add3DRect(-0.96f * innerRadius, -0.3f * innerRadius - 0.5f * barWidth, -0.3f * innerRadius - 0.5f * barWidth, -0.3f * innerRadius + 0.5f * barWidth); // 左下

    return vertices;
}

// 生成带圆形开口的墙壁顶点
std::vector<float> generateWallWithHoleVertices(float width, float height, float holeRadius, int segments)
{
    std::vector<float> vertices;
    const float twoPI = 2.0f * static_cast<float>(M_PI);
    float halfW = width / 2.0f;
    float halfH = height / 2.0f;

    for (int i = 0; i < segments; ++i)
    {
        float angle1 = twoPI * static_cast<float>(i) / static_cast<float>(segments);
        float angle2 = twoPI * static_cast<float>(i + 1) / static_cast<float>(segments);

        // 内圈顶点 (洞的边缘)
        glm::vec3 inner1(holeRadius * cosf(angle1), holeRadius * sinf(angle1), 0.0f);
        glm::vec3 inner2(holeRadius * cosf(angle2), holeRadius * sinf(angle2), 0.0f);

        // 外圈顶点 (墙壁的边缘)
        // 使用一个足够大的矩形来确保外顶点，然后裁剪到墙壁的实际边界
        float outer_x1 = std::max(-halfW, std::min(halfW, inner1.x * 100)); // 将内点投影到边上
        float outer_y1 = std::max(-halfH, std::min(halfH, inner1.y * 100));
        // 处理角落，确保它们正确地在角落上
        if (abs(outer_x1) == halfW && abs(outer_y1) > 0) outer_y1 = (inner1.y > 0) ? halfH : -halfH;
        if (abs(outer_y1) == halfH && abs(outer_x1) > 0) outer_x1 = (inner1.x > 0) ? halfW : -halfW;
        
        float outer_x2 = std::max(-halfW, std::min(halfW, inner2.x * 100));
        float outer_y2 = std::max(-halfH, std::min(halfH, inner2.y * 100));
        if (abs(outer_x2) == halfW && abs(outer_y2) > 0) outer_y2 = (inner2.y > 0) ? halfH : -halfH;
        if (abs(outer_y2) == halfH && abs(outer_x2) > 0) outer_x2 = (inner2.x > 0) ? halfW : -halfW;


        glm::vec3 outer1(outer_x1, outer_y1, 0.0f);
        glm::vec3 outer2(outer_x2, outer_y2, 0.0f);
        
        // 法线，因为是后墙，所以朝向Z轴正方向
        glm::vec3 normal(0.0f, 0.0f, 1.0f);

        // 用两个三角形构成一个四边形
        // 三角形 1
        vertices.insert(vertices.end(), {inner1.x, inner1.y, inner1.z, normal.x, normal.y, normal.z});
        vertices.insert(vertices.end(), {outer1.x, outer1.y, outer1.z, normal.x, normal.y, normal.z});
        vertices.insert(vertices.end(), {outer2.x, outer2.y, outer2.z, normal.x, normal.y, normal.z});

        // 三角形 2
        vertices.insert(vertices.end(), {inner1.x, inner1.y, inner1.z, normal.x, normal.y, normal.z});
        vertices.insert(vertices.end(), {outer2.x, outer2.y, outer2.z, normal.x, normal.y, normal.z});
        vertices.insert(vertices.end(), {inner2.x, inner2.y, inner2.z, normal.x, normal.y, normal.z});
    }

    return vertices;
}