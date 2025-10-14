#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "shader.h"
#include "camera.h"
#define STB_IMAGE_IMPLEMENTATION
#include "model.h"

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
// 生成中式镂空窗
std::vector<float> generateCircularWindowVertices(int segments, float outerRadius, float innerRadius, float barWidth, float depth);
// 生成带洞墙体
std::vector<float> generateWallWithHoleVertices(float width, float height, float holeRadius, int segments);

// 窗口设置
const unsigned int SCR_WIDTH = 1600;
const unsigned int SCR_HEIGHT = 1200;

// 摄像机设置
Camera camera(glm::vec3(0.0f, 0.0f, 5.0f));
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;

// 时间设置
float deltaTime = 0.0f;
float lastFrame = 0.0f;

// 光照设置
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

    // glfw创建窗口
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

    // 告诉 GLFW 捕获我们的鼠标
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // glad：加载所有 OpenGL 函数指针
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

    // 编译shader操作
    // ------------------------------------
    Shader lightingShader("lighting.vs", "lighting.fs");
    Shader lightCubeShader("lightcube.vs", "lightcube.fs");
    Shader modelShader("model_loading.vs", "model_loading.fs");
    
    // --- 加载模型 ---
    Model ourModel("resource/model/table3.obj");
    Model lampModel("resource/lamp/lamp1.obj");

    // 统一设置用到的坐标信息(每一行前三个数字为点的坐标，后三个为法向量)
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

    // --- 生成圆形窗户的顶点数据 ---
    // ------------------------------------------------------------------
    float windowOuterRadius = 1.2f;
    float windowScale = 1.2f;
    // std::vector<float> circularWindowVertices = generateCircularWindowVertices(72, windowOuterRadius, 1.1f, 0.05f);
    std::vector<float> circularWindowVertices = generateCircularWindowVertices(72, windowOuterRadius, 1.1f, 0.05f, 0.1f);    
    int windowVertexCount = circularWindowVertices.size() / 6;

    // --- 生成带洞后墙的顶点数据 ---
    std::vector<float> wallWithHoleVertices = generateWallWithHoleVertices(8.0f, 6.0f, windowOuterRadius * windowScale, 72);
    int wallWithHoleVertexCount = wallWithHoleVertices.size() / 6;

    // 房间墙壁、地板、天花板的VAO/VBO
    // ------------------------------------------------------------------
    unsigned int roomVAO, roomVBO;
    glGenVertexArrays(1, &roomVAO);
    glGenBuffers(1, &roomVBO);
    glBindVertexArray(roomVAO);
    glBindBuffer(GL_ARRAY_BUFFER, roomVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    // 载入位置
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    // 载入法向量
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    // 窗户的VAO/VBO
    // ------------------------------------------------------------------
    unsigned int windowVAO, windowVBO;
    glGenVertexArrays(1, &windowVAO);
    glGenBuffers(1, &windowVBO);
    glBindVertexArray(windowVAO);
    glBindBuffer(GL_ARRAY_BUFFER, windowVBO);
    glBufferData(GL_ARRAY_BUFFER, circularWindowVertices.size() * sizeof(float), circularWindowVertices.data(), GL_STATIC_DRAW);
    // 载入位置
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    // 载入法向量
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    // 载入方块灯的顶点信息
    // ------------------------------------------------------------------
    unsigned int lightCubeVAO;
    glGenVertexArrays(1, &lightCubeVAO);
    glBindVertexArray(lightCubeVAO);
    // 只需绑定VBO, 其中的数据包含了所需的顶点
    glBindBuffer(GL_ARRAY_BUFFER, roomVBO);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    
    // 带洞后墙的VAO/VBO
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

        // 光源位置固定在台灯处
        glm::vec3 lampLightPos = glm::vec3(-1.0f, -1.1f, -2.5f);

        // 定义昏暗的暖色光的基础颜色和强度
        glm::vec3 warmColor(1.0f, 0.85f, 0.6f);
        float overallIntensity = 0.6f; // 整体亮度因子
        glm::vec3 finalLightColor = warmColor * overallIntensity;

        // 确保在设置 Uniforms/Drawing 对象时激活 Shader
        //---------------------------------------------------------------------
        lightingShader.use();
        lightingShader.setVec3("lightPos", lampLightPos);
        lightingShader.setVec3("viewPos", camera.Position);
        lightingShader.setVec3("lightColor", finalLightColor);

        glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
        glm::mat4 view = camera.GetViewMatrix();
        lightingShader.setMat4("projection", projection);
        lightingShader.setMat4("view", view);

        glm::mat4 model = glm::mat4(1.0f);

        glBindVertexArray(roomVAO);

        //绘制天花板
        {
            //设置光照参数
            lightingShader.setVec3("objectColor", 0.5, 0.5f, 0.5f);

            // 世界坐标变换
            model = glm::mat4(1.0f);
            model = glm::translate(model, glm::vec3(0.0f, 3.0f, 0.0f));
            model = glm::scale(model, glm::vec3(8.0f, 0.1f, 8.0f));
            lightingShader.setMat4("model", model);

            // 渲染
            glDrawArrays(GL_TRIANGLES, 0, 36);
        }

        // 绘制地板
        {
            //设置光照参数
            lightingShader.setVec3("objectColor", 0.4f, 0.3f, 0.25f);

            // 世界坐标变换
            model = glm::mat4(1.0f);
            model = glm::translate(model, glm::vec3(0.0f, -3.0f, 0.0f));
            model = glm::scale(model, glm::vec3(8.0f, 0.1f, 8.0f));
            lightingShader.setMat4("model", model);

            // 渲染
            glDrawArrays(GL_TRIANGLES, 0, 36);
        }

        // 绘制左墙
        {
            //设置光照参数
            lightingShader.setVec3("objectColor", 0.9f, 0.85f, 0.7f);

            // 世界坐标变换
            model = glm::mat4(1.0f);
            model = glm::translate(model, glm::vec3(-4.0f, 0.0f, 0.0f));
            model = glm::scale(model, glm::vec3(0.1f, 6.0f, 8.0f));
            lightingShader.setMat4("model", model);

            // 渲染
            glDrawArrays(GL_TRIANGLES, 0, 36);
        }

        // 绘制右墙
        {
            //设置光照参数
            lightingShader.setVec3("objectColor", 0.9f, 0.85f, 0.7f);

            // 世界坐标变换
            model = glm::mat4(1.0f);
            model = glm::translate(model, glm::vec3(4.0f, 0.0f, 0.0f));
            model = glm::scale(model, glm::vec3(0.1f, 6.0f, 8.0f));
            lightingShader.setMat4("model", model);

            // 渲染
            glDrawArrays(GL_TRIANGLES, 0, 36);
        }

        // // 绘制后墙
        // {
        //     //设置光照参数
        //     lightingShader.setVec3("objectColor", 1.0f, 1.0f, 1.0f);

        //     // 世界坐标变换
        //     model = glm::mat4(1.0f);
        //     model = glm::translate(model, glm::vec3(0.0f, 0.0f, -4.0f));
        //     model = glm::scale(model, glm::vec3(8.0f, 6.0f, 0.1f));
        //     lightingShader.setMat4("model", model);

        //     // 渲染
        //     glDrawArrays(GL_TRIANGLES, 0, 36);
        // }
        
        // 绘制带洞的后墙
        {
            //设置光照参数
            lightingShader.setVec3("objectColor", 1.0f, 1.0f, 1.0f);

            // 世界坐标变换
            model = glm::mat4(1.0f);
            model = glm::translate(model, glm::vec3(0.0f, 0.0f, -4.0f));
            // 注意：这里不再需要 scale，因为顶点数据已经定义了正确的尺寸
            lightingShader.setMat4("model", model);

            // 渲染带洞的墙
            glBindVertexArray(wallVAO);
            glDrawArrays(GL_TRIANGLES, 0, wallWithHoleVertexCount);
        }

        // 绘制窗户
        {
            //设置光照参数
            lightingShader.setVec3("objectColor", 0.36, 0.2f, 0.09f); // 木质颜色

            // 世界坐标变换
            model = glm::mat4(1.0f);
            model = glm::translate(model, glm::vec3(0.0f, 0.0f, -4.0f)); // 放置在后墙前一点
            model = glm::scale(model, glm::vec3(1.2f));
            lightingShader.setMat4("model", model);
            glBindVertexArray(windowVAO);
            glDrawArrays(GL_TRIANGLES, 0, windowVertexCount); 
        }

        // // 绘制灯
        // {
        //     lightCubeShader.use();
        //     lightCubeShader.setMat4("projection", projection);
        //     lightCubeShader.setMat4("view", view);
        //     model = glm::mat4(1.0f);
        //     model = glm::translate(model, lightPos);
        //     model = glm::scale(model, glm::vec3(0.2f)); // a smaller cube
        //     lightCubeShader.setMat4("model", model);

        //     glBindVertexArray(lightCubeVAO);
        //     glDrawArrays(GL_TRIANGLES, 0, 36);
        // }

        // --- 绘制书桌模型 ---
        {
            modelShader.use();
            modelShader.setMat4("projection", projection);
            modelShader.setMat4("view", view);

            // 设置光照
            modelShader.setVec3("viewPos", camera.Position);
            modelShader.setVec3("light.position", lampLightPos);
            modelShader.setVec3("light.ambient", finalLightColor * 0.2f);
            modelShader.setVec3("light.diffuse", finalLightColor);
            modelShader.setVec3("light.specular", finalLightColor * 0.2f);
        
            // 渲染加载的模型
            model = glm::mat4(1.0f);
            model = glm::translate(model, glm::vec3(0.0f, -3.0f, -2.5f)); // 放在地板上，靠窗
            model = glm::scale(model, glm::vec3(0.5f, 0.5f, 0.5f));	// 适当缩放模型
            model = glm::rotate(model, glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f)); // 旋转90度，使其正对前方
            modelShader.setMat4("model", model);
            ourModel.Draw(modelShader);
        }

        // --- 绘制台灯模型 ---
        {
            // modelShader 已经被激活，且 view/projection/light 等 uniform 已设置
            // 我们只需要为台灯设置一个新的 model 矩阵
            modelShader.use(); 
            model = glm::mat4(1.0f);
            // 将台灯放在书桌表面上偏左的位置
            model = glm::translate(model, glm::vec3(-1.0f, -0.8f, -3.0f)); 
            model = glm::scale(model, glm::vec3(0.3f)); // 缩放台灯使其尺寸合适
            modelShader.setMat4("model", model);
            lampModel.Draw(modelShader);
            
        }

        // glfw：交换缓冲区和轮询 IO 事件（按下/释放键、移动鼠标等）
        // -------------------------------------------------------------------------------
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // （可选）一旦资源超出其用途，就取消分配所有资源：
    // ------------------------------------------------------------------------
    glDeleteVertexArrays(1, &roomVAO);
    glDeleteVertexArrays(1, &lightCubeVAO);
    glDeleteVertexArrays(1, &windowVAO);
    glDeleteVertexArrays(1, &wallVAO);
    glDeleteBuffers(1, &roomVBO);
    glDeleteBuffers(1, &windowVBO);
    glDeleteBuffers(1, &wallVBO);

    // glfw：终止，清除所有以前分配的 GLFW 资源。
    // ------------------------------------------------------------------
    glfwTerminate();
    return 0;
}

//查询 GLFW 是否按下/释放了该帧的相关键并做出相应的反应
// ---------------------------------------------------------------------------------------------------------
void processInput(GLFWwindow* window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        camera.ProcessKeyboard(FORWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        camera.ProcessKeyboard(BACKWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        camera.ProcessKeyboard(LEFT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        camera.ProcessKeyboard(RIGHT, deltaTime);
}

// glfw：每当窗口大小发生变化（通过操作系统或用户调整大小）时，此回调函数都会执行
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    // 确保视区与新的窗口尺寸匹配;请注意，width和height将明显大于 Retina 显示屏上指定的高度
    glViewport(0, 0, width, height);
}


// glfw: 每当鼠标移动时，该回调都会被调用
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
    float yoffset = lastY - ypos; // 反转，因为 y 坐标从下到上

    lastX = xpos;
    lastY = ypos;

    camera.ProcessMouseMovement(xoffset, yoffset);
}

// glfw:每当鼠标滚轮滚动时，该回调都会被调用
// ----------------------------------------------------------------------
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    camera.ProcessMouseScroll(static_cast<float>(yoffset));
}

// 将生成圆形窗户顶点的逻辑封装成一个独立的函数
std::vector<float> generateCircularWindowVertices(int segments, float outerRadius, float innerRadius, float barWidth, float depth)
{
    std::vector<float> vertices;
    const float twoPI = 2.0f * static_cast<float>(M_PI);
    float halfDepth = depth / 2.0f;

    // 辅助函数，用于添加一个完整的3D条带片段 (用于曲线)
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
    
    // 辅助函数，用于添加一个完整的3D矩形 (用于直线)
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

    // 1. 主圆环（annulus）
    for (int i = 0; i < segments; ++i) {
        float angle1 = twoPI * static_cast<float>(i) / static_cast<float>(segments);
        float angle2 = twoPI * static_cast<float>(i + 1) / static_cast<float>(segments);
        
        glm::vec2 p1_outer(outerRadius * cosf(angle1), outerRadius * sinf(angle1));
        glm::vec2 p2_outer(outerRadius * cosf(angle2), outerRadius * sinf(angle2));
        glm::vec2 p1_inner(innerRadius * cosf(angle1), innerRadius * sinf(angle1));
        glm::vec2 p2_inner(innerRadius * cosf(angle2), innerRadius * sinf(angle2));
        
        add3DStrip(p1_inner, p1_outer, p2_inner, p2_outer);
    }

    // 2. 内部十字格栅
    add3DRect(-innerRadius, -barWidth, innerRadius, barWidth); // 水平条
    add3DRect(-barWidth, -innerRadius, barWidth, innerRadius); // 垂直条

    // 3. 生成首尾相连的四个半圆花纹
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
    
    // 4.生成中部八边形 和四个正方形
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

    // 四条斜边
    // (为简化，斜边用一个近似的矩形表示，视觉上差异很小)
    add3DRect(0.2f * innerRadius, 0.4f * innerRadius - barWidth, 0.4f * innerRadius + barWidth, 0.2f * innerRadius);
    add3DRect(-0.4f * innerRadius - barWidth, 0.2f * innerRadius, -0.2f * innerRadius, 0.4f * innerRadius + barWidth);
    add3DRect(-0.4f * innerRadius - barWidth, -0.2f * innerRadius, -0.2f * innerRadius, -0.4f * innerRadius - barWidth);
    add3DRect(0.2f * innerRadius, -0.4f * innerRadius - barWidth, 0.4f * innerRadius + barWidth, -0.2f * innerRadius);

    // 在每个画出的正方形中心画一横一竖的直线
    add3DRect(-0.85f * innerRadius, 0.6f * innerRadius - 0.5f * barWidth, 0.85f * innerRadius, 0.6f * innerRadius + 0.5f * barWidth); // 上
    add3DRect(-0.85f * innerRadius, -0.6f * innerRadius - 0.5f * barWidth, 0.85f * innerRadius, -0.6f * innerRadius + 0.5f * barWidth); // 下
    add3DRect(-0.6f * innerRadius - 0.5f * barWidth, -0.85f * innerRadius, -0.6f * innerRadius + 0.5f * barWidth, 0.85f * innerRadius); // 左
    add3DRect(0.6f * innerRadius - 0.5f * barWidth, -0.85f * innerRadius, 0.6f * innerRadius + 0.5f * barWidth, 0.85f * innerRadius); // 右

    // 从每条斜线的中点开始, 向外引一条直线
    // (为简化，同样用近似的矩形表示)
    add3DRect(-0.3f * innerRadius - 0.5f * barWidth, 0.3f * innerRadius + 0.5f * barWidth, -0.3f * innerRadius + 0.5f * barWidth, 0.96f * innerRadius); // 上左
    add3DRect(0.3f * innerRadius - 0.5f * barWidth, 0.3f * innerRadius + 0.5f * barWidth, 0.3f * innerRadius + 0.5f * barWidth, 0.96f * innerRadius); // 上右
    add3DRect(-0.3f * innerRadius - 0.5f * barWidth, -0.96f * innerRadius, -0.3f * innerRadius + 0.5f * barWidth, -0.3f * innerRadius - 0.5f * barWidth); // 下左
    add3DRect(0.3f * innerRadius - 0.5f * barWidth, -0.96f * innerRadius, 0.3f * innerRadius + 0.5f * barWidth, -0.3f * innerRadius - 0.5f * barWidth); // 下右
    add3DRect(0.3f * innerRadius + 0.5f * barWidth, 0.3f * innerRadius - 0.5f * barWidth, 0.96f * innerRadius, 0.3f * innerRadius + 0.5f * barWidth); // 右上
    add3DRect(0.3f * innerRadius + 0.5f * barWidth, -0.3f * innerRadius - 0.5f * barWidth, 0.96f * innerRadius, -0.3f * innerRadius + 0.5f * barWidth); // 右下
    add3DRect(-0.96f * innerRadius, 0.3f * innerRadius - 0.5f * barWidth, -0.3f * innerRadius - 0.5f * barWidth, 0.3f * innerRadius + 0.5f * barWidth); // 左上
    add3DRect(-0.96f * innerRadius, -0.3f * innerRadius - 0.5f * barWidth, -0.3f * innerRadius - 0.5f * barWidth, -0.3f * innerRadius + 0.5f * barWidth); // 左下

    return vertices;
}

// 生成带圆形孔洞的墙壁顶点
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

        // 内圈顶点 (洞口边缘)
        glm::vec3 inner1(holeRadius * cosf(angle1), holeRadius * sinf(angle1), 0.0f);
        glm::vec3 inner2(holeRadius * cosf(angle2), holeRadius * sinf(angle2), 0.0f);

        // 外圈顶点 (墙壁边缘)
        // 使用一个足够大的外接矩形来确定外顶点，然后将其裁剪到墙的实际边界
        float outer_x1 = std::max(-halfW, std::min(halfW, inner1.x * 100)); // 乘以大数以投射到边缘
        float outer_y1 = std::max(-halfH, std::min(halfH, inner1.y * 100));
        // 如果点在墙角，确保它精确地在角上
        if (abs(outer_x1) == halfW && abs(outer_y1) > 0) outer_y1 = (inner1.y > 0) ? halfH : -halfH;
        if (abs(outer_y1) == halfH && abs(outer_x1) > 0) outer_x1 = (inner1.x > 0) ? halfW : -halfW;
        
        float outer_x2 = std::max(-halfW, std::min(halfW, inner2.x * 100));
        float outer_y2 = std::max(-halfH, std::min(halfH, inner2.y * 100));
        if (abs(outer_x2) == halfW && abs(outer_y2) > 0) outer_y2 = (inner2.y > 0) ? halfH : -halfH;
        if (abs(outer_y2) == halfH && abs(outer_x2) > 0) outer_x2 = (inner2.x > 0) ? halfW : -halfW;


        glm::vec3 outer1(outer_x1, outer_y1, 0.0f);
        glm::vec3 outer2(outer_x2, outer_y2, 0.0f);
        
        // 法线，对于后墙，朝向正Z轴
        glm::vec3 normal(0.0f, 0.0f, 1.0f);

        // 用两个三角形构成一个梯形
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