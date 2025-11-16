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
// ������ʽ�οմ�
std::vector<float> generateCircularWindowVertices(int segments, float outerRadius, float innerRadius, float barWidth, float depth);
// ���ɴ���ǽ��
std::vector<float> generateWallWithHoleVertices(float width, float height, float holeRadius, int segments);

// ��������
const unsigned int SCR_WIDTH = 1600;
const unsigned int SCR_HEIGHT = 1200;

// ���������
Camera camera(glm::vec3(0.0f, 0.0f, 5.0f));
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;

// ʱ������
float deltaTime = 0.0f;
float lastFrame = 0.0f;

// ��������
glm::vec3 lightPos(0.0f, 2.5f, 3.0f);

int main()
{
    // ��ʼ��������glfw
    // ------------------------------
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    // glfw��������
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

    // ���� GLFW �������ǵ����
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // glad���������� OpenGL ����ָ��
    // ---------------------------------------
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }
    stbi_set_flip_vertically_on_load(true);

    // ����ȫ�� OpenGL ״̬
    // -----------------------------
    glEnable(GL_DEPTH_TEST);

    // Shader
    // ------------------------------------
    Shader lightingShader("lighting.vs", "lighting.fs");
    Shader lightCubeShader("lightcube.vs", "lightcube.fs");
    Shader modelShader("model_loading.vs", "model_loading.fs");
    Shader sandboxShader("sandbox.vs", "sandbox.fs");
    
    // --- 加载模型 ---
    Model ourModel("resource/model/table3.obj");
    Model lampModel("resource/lamp/lamp1.obj");

    // --- 创建沙盘实例 ---
    // 方法1：从高度图创建
    TerrainSandbox sandbox_heightmap(
        1.5f, // 宽度
        1.0f, // 深度
        128,  // 网格精度
        TerrainSandbox::GenMethod::HEIGHTMAP,
        "resource/textures/heightmap1.png", // 高度图路径
        "resource/textures/red_sand_diff_4k.jpg"  // 沙子纹理路径
    );

    // 方法2：程序化随机生成
    TerrainSandbox sandbox_procedural(
        1.5f, 1.0f, 64,
        TerrainSandbox::GenMethod::PROCEDURAL_RANDOM,
        nullptr,
        "resource/textures/red_sand_diff_4k.jpg"
    );


    // ͳһ�����õ���������Ϣ(ÿһ��ǰ��������Ϊ������꣬������Ϊ������)
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

    // --- ����Բ�δ����Ķ������� ---
    // ------------------------------------------------------------------
    float windowOuterRadius = 1.2f;
    float windowScale = 1.2f;
    // std::vector<float> circularWindowVertices = generateCircularWindowVertices(72, windowOuterRadius, 1.1f, 0.05f);
    std::vector<float> circularWindowVertices = generateCircularWindowVertices(72, windowOuterRadius, 1.1f, 0.05f, 0.1f);    
    int windowVertexCount = circularWindowVertices.size() / 6;

    // --- ���ɴ�����ǽ�Ķ������� ---
    std::vector<float> wallWithHoleVertices = generateWallWithHoleVertices(8.0f, 6.0f, windowOuterRadius * windowScale, 72);
    int wallWithHoleVertexCount = wallWithHoleVertices.size() / 6;

    // ����ǽ�ڡ��ذ塢�컨���VAO/VBO
    // ------------------------------------------------------------------
    unsigned int roomVAO, roomVBO;
    glGenVertexArrays(1, &roomVAO);
    glGenBuffers(1, &roomVBO);
    glBindVertexArray(roomVAO);
    glBindBuffer(GL_ARRAY_BUFFER, roomVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    // ����λ��
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    // ���뷨����
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    // ������VAO/VBO
    // ------------------------------------------------------------------
    unsigned int windowVAO, windowVBO;
    glGenVertexArrays(1, &windowVAO);
    glGenBuffers(1, &windowVBO);
    glBindVertexArray(windowVAO);
    glBindBuffer(GL_ARRAY_BUFFER, windowVBO);
    glBufferData(GL_ARRAY_BUFFER, circularWindowVertices.size() * sizeof(float), circularWindowVertices.data(), GL_STATIC_DRAW);
    // ����λ��
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    // ���뷨����
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    // ���뷽��ƵĶ�����Ϣ
    // ------------------------------------------------------------------
    unsigned int lightCubeVAO;
    glGenVertexArrays(1, &lightCubeVAO);
    glBindVertexArray(lightCubeVAO);
    // ֻ���VBO, ���е����ݰ���������Ķ���
    glBindBuffer(GL_ARRAY_BUFFER, roomVBO);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    
    // ������ǽ��VAO/VBO
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

    // ��Ⱦѭ��
    // -----------
    while (!glfwWindowShouldClose(window))
    {
        // ʱ���߼�
        // --------------------
        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // ����
        // -----
        processInput(window);

        // ��ʼ��Ⱦ
        // ------
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // ��Դλ�ù̶���̨�ƴ�
        glm::vec3 lampLightPos = glm::vec3(-1.0f, -1.1f, -2.5f);

        // ����谵��ůɫ��Ļ�����ɫ��ǿ��
        glm::vec3 warmColor(1.0f, 0.85f, 0.6f);
        float overallIntensity = 0.6f; // ������������
        glm::vec3 finalLightColor = warmColor * overallIntensity;

        // ȷ�������� Uniforms/Drawing ����ʱ���� Shader
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

        // 台灯模型
        {
            //���ù��ղ���
            lightingShader.setVec3("objectColor", 0.5, 0.5f, 0.5f);

            // ��������任
            model = glm::mat4(1.0f);
            model = glm::translate(model, glm::vec3(0.0f, 3.0f, 0.0f));
            model = glm::scale(model, glm::vec3(8.0f, 0.1f, 8.0f));
            lightingShader.setMat4("model", model);

            // ��Ⱦ
            glDrawArrays(GL_TRIANGLES, 0, 36);
        }

        // ���Ƶذ�
        {
            //���ù��ղ���
            lightingShader.use();
            lightingShader.setVec3("objectColor", 0.4f, 0.3f, 0.25f);

            // ��������任
            model = glm::mat4(1.0f);
            model = glm::translate(model, glm::vec3(0.0f, -3.0f, 0.0f));
            model = glm::scale(model, glm::vec3(8.0f, 0.1f, 8.0f));
            lightingShader.setMat4("model", model);

            // ��Ⱦ
            glDrawArrays(GL_TRIANGLES, 0, 36);
        }

        // ������ǽ
        {
            //���ù��ղ���
            lightingShader.use();
            lightingShader.setVec3("objectColor", 0.9f, 0.85f, 0.7f);

            // ��������任
            model = glm::mat4(1.0f);
            model = glm::translate(model, glm::vec3(-4.0f, 0.0f, 0.0f));
            model = glm::scale(model, glm::vec3(0.1f, 6.0f, 8.0f));
            lightingShader.setMat4("model", model);

            // ��Ⱦ
            glDrawArrays(GL_TRIANGLES, 0, 36);
        }

        // ������ǽ
        {
            //���ù��ղ���
            lightingShader.use();
            lightingShader.setVec3("objectColor", 0.9f, 0.85f, 0.7f);

            // ��������任
            model = glm::mat4(1.0f);
            model = glm::translate(model, glm::vec3(4.0f, 0.0f, 0.0f));
            model = glm::scale(model, glm::vec3(0.1f, 6.0f, 8.0f));
            lightingShader.setMat4("model", model);

            // ��Ⱦ
            glDrawArrays(GL_TRIANGLES, 0, 36);
        }

        // // ���ƺ�ǽ
        // {
        //     //���ù��ղ���
        //     lightingShader.setVec3("objectColor", 1.0f, 1.0f, 1.0f);

        //     // ��������任
        //     model = glm::mat4(1.0f);
        //     model = glm::translate(model, glm::vec3(0.0f, 0.0f, -4.0f));
        //     model = glm::scale(model, glm::vec3(8.0f, 6.0f, 0.1f));
        //     lightingShader.setMat4("model", model);

        //     // ��Ⱦ
        //     glDrawArrays(GL_TRIANGLES, 0, 36);
        // }
        
        // ���ƴ����ĺ�ǽ
        {
            //���ù��ղ���
            lightingShader.use();
            lightingShader.setVec3("objectColor", 1.0f, 1.0f, 1.0f);

            // ��������任
            model = glm::mat4(1.0f);
            model = glm::translate(model, glm::vec3(0.0f, 0.0f, -4.0f));
            // ע�⣺���ﲻ����Ҫ scale����Ϊ���������Ѿ���������ȷ�ĳߴ�
            lightingShader.setMat4("model", model);

            // ��Ⱦ������ǽ
            glBindVertexArray(wallVAO);
            glDrawArrays(GL_TRIANGLES, 0, wallWithHoleVertexCount);
        }

        // ���ƴ���
        {
            //���ù��ղ���
            lightingShader.use();
            lightingShader.setVec3("objectColor", 0.36, 0.2f, 0.09f); // ľ����ɫ

            // ��������任
            model = glm::mat4(1.0f);
            model = glm::translate(model, glm::vec3(0.0f, 0.0f, -4.0f)); // �����ں�ǽǰһ��
            model = glm::scale(model, glm::vec3(1.2f));
            lightingShader.setMat4("model", model);
            glBindVertexArray(windowVAO);
            glDrawArrays(GL_TRIANGLES, 0, windowVertexCount); 
        }

        // // ���Ƶ�
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

        // --- ��������ģ�� ---
        {
            modelShader.use();
            modelShader.setMat4("projection", projection);
            modelShader.setMat4("view", view);

            // ���ù���
            modelShader.setVec3("viewPos", camera.Position);
            modelShader.setVec3("light.position", lampLightPos);
            modelShader.setVec3("light.ambient", finalLightColor * 0.2f);
            modelShader.setVec3("light.diffuse", finalLightColor);
            modelShader.setVec3("light.specular", finalLightColor * 0.2f);
        
            // ��Ⱦ���ص�ģ��
            model = glm::mat4(1.0f);
            model = glm::translate(model, glm::vec3(0.0f, -3.0f, -2.5f)); // ���ڵذ��ϣ�����
            model = glm::scale(model, glm::vec3(0.5f, 0.5f, 0.5f));	// �ʵ�����ģ��
            model = glm::rotate(model, glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f)); // ��ת90�ȣ�ʹ������ǰ��
            modelShader.setMat4("model", model);
            ourModel.Draw(modelShader);
        }

        // --- ����̨��ģ�� ---
        {
            // modelShader �Ѿ�������� view/projection/light �� uniform ������
            // ����ֻ��ҪΪ̨������һ���µ� model ����
            modelShader.use(); 
            model = glm::mat4(1.0f);
            // ��̨�Ʒ�������������ƫ���λ��
            model = glm::translate(model, glm::vec3(-1.0f, -0.8f, -3.0f)); 
            model = glm::scale(model, glm::vec3(0.3f)); // ����̨��ʹ��ߴ����
            modelShader.setMat4("model", model);
            lampModel.Draw(modelShader);
            
        }

        // --- 绘制地形沙盘 ---
        {
            sandboxShader.use();
            sandboxShader.setMat4("projection", projection);
            sandboxShader.setMat4("view", view);
            sandboxShader.setVec3("viewPos", camera.Position);
            sandboxShader.setVec3("lightPos", lampLightPos);
            sandboxShader.setVec3("lightColor", finalLightColor);

            model = glm::mat4(1.0f);
            // 将沙盘放在书桌上
            model = glm::translate(model, glm::vec3(0.5f, -1.4f, -3.0f));
            sandboxShader.setMat4("model", model);

            // 绘制沙盘 (根据选择的方法)
            sandbox_heightmap.Draw(sandboxShader);
            // sandbox_procedural.Draw(sandboxShader);
        }



        // glfw����������������ѯ IO �¼�������/�ͷż����ƶ����ȣ�
        // -------------------------------------------------------------------------------
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // ����ѡ��һ����Դ��������;����ȡ������������Դ��
    // ------------------------------------------------------------------------
    glDeleteVertexArrays(1, &roomVAO);
    glDeleteVertexArrays(1, &lightCubeVAO);
    glDeleteVertexArrays(1, &windowVAO);
    glDeleteVertexArrays(1, &wallVAO);
    glDeleteBuffers(1, &roomVBO);
    glDeleteBuffers(1, &windowVBO);
    glDeleteBuffers(1, &wallVBO);

    // glfw����ֹ�����������ǰ����� GLFW ��Դ��
    // ------------------------------------------------------------------
    glfwTerminate();
    return 0;
}

//��ѯ GLFW �Ƿ���/�ͷ��˸�֡����ؼ���������Ӧ�ķ�Ӧ
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

// glfw��ÿ�����ڴ�С�����仯��ͨ������ϵͳ���û�������С��ʱ���˻ص���������ִ��
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    // ȷ���������µĴ��ڳߴ�ƥ��;��ע�⣬width��height�����Դ��� Retina ��ʾ����ָ���ĸ߶�
    glViewport(0, 0, width, height);
}


// glfw: ÿ������ƶ�ʱ���ûص����ᱻ����
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
    float yoffset = lastY - ypos; // ��ת����Ϊ y ������µ���

    lastX = xpos;
    lastY = ypos;

    camera.ProcessMouseMovement(xoffset, yoffset);
}

// glfw:ÿ�������ֹ���ʱ���ûص����ᱻ����
// ----------------------------------------------------------------------
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    camera.ProcessMouseScroll(static_cast<float>(yoffset));
}

// ������Բ�δ���������߼���װ��һ�������ĺ���
std::vector<float> generateCircularWindowVertices(int segments, float outerRadius, float innerRadius, float barWidth, float depth)
{
    std::vector<float> vertices;
    const float twoPI = 2.0f * static_cast<float>(M_PI);
    float halfDepth = depth / 2.0f;

    // ������������������һ��������3D����Ƭ�� (��������)
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
    
    // ������������������һ��������3D���� (����ֱ��)
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

    // 1. ��Բ����annulus��
    for (int i = 0; i < segments; ++i) {
        float angle1 = twoPI * static_cast<float>(i) / static_cast<float>(segments);
        float angle2 = twoPI * static_cast<float>(i + 1) / static_cast<float>(segments);
        
        glm::vec2 p1_outer(outerRadius * cosf(angle1), outerRadius * sinf(angle1));
        glm::vec2 p2_outer(outerRadius * cosf(angle2), outerRadius * sinf(angle2));
        glm::vec2 p1_inner(innerRadius * cosf(angle1), innerRadius * sinf(angle1));
        glm::vec2 p2_inner(innerRadius * cosf(angle2), innerRadius * sinf(angle2));
        
        add3DStrip(p1_inner, p1_outer, p2_inner, p2_outer);
    }

    // 2. �ڲ�ʮ�ָ�դ
    add3DRect(-innerRadius, -barWidth, innerRadius, barWidth); // ˮƽ��
    add3DRect(-barWidth, -innerRadius, barWidth, innerRadius); // ��ֱ��

    // 3. ������β�������ĸ���Բ����
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
    
    // 4.�����в��˱��� ���ĸ�������
    // ��
    add3DRect(-0.2f * innerRadius, 0.4f * innerRadius - barWidth, 0.2f * innerRadius, 0.4f * innerRadius + barWidth);
    add3DRect(-0.2f * innerRadius, 0.8f * innerRadius - barWidth, 0.2f * innerRadius, 0.8f * innerRadius);
    add3DRect(0.2f * innerRadius - barWidth, 0.4f * innerRadius, 0.2f * innerRadius, 0.8f * innerRadius);
    add3DRect(-0.2f * innerRadius, 0.4f * innerRadius, -0.2f * innerRadius + barWidth, 0.8f * innerRadius);
    // ��
    add3DRect(-0.2f * innerRadius, -0.4f * innerRadius - barWidth, 0.2f * innerRadius, -0.4f * innerRadius + barWidth);
    add3DRect(-0.2f * innerRadius, -0.8f * innerRadius - barWidth, 0.2f * innerRadius, -0.8f * innerRadius);
    add3DRect(0.2f * innerRadius - barWidth, -0.8f * innerRadius, 0.2f * innerRadius, -0.4f * innerRadius);
    add3DRect(-0.2f * innerRadius, -0.8f * innerRadius, -0.2f * innerRadius + barWidth, -0.4f * innerRadius);
    // ��
    add3DRect(0.4f * innerRadius - barWidth, -0.2f * innerRadius, 0.4f * innerRadius + barWidth, 0.2f * innerRadius);
    add3DRect(0.8f * innerRadius - barWidth, -0.2f * innerRadius, 0.8f * innerRadius, 0.2f * innerRadius);
    add3DRect(0.4f * innerRadius, 0.2f * innerRadius - barWidth, 0.8f * innerRadius, 0.2f * innerRadius);
    add3DRect(0.4f * innerRadius, -0.2f * innerRadius, 0.8f * innerRadius, -0.2f * innerRadius + barWidth);
    // ��
    add3DRect(-0.4f * innerRadius - barWidth, -0.2f * innerRadius, -0.4f * innerRadius + barWidth, 0.2f * innerRadius);
    add3DRect(-0.8f * innerRadius - barWidth, -0.2f * innerRadius, -0.8f * innerRadius, 0.2f * innerRadius);
    add3DRect(-0.8f * innerRadius, 0.2f * innerRadius - barWidth, -0.4f * innerRadius, 0.2f * innerRadius);
    add3DRect(-0.8f * innerRadius, -0.2f * innerRadius, -0.4f * innerRadius, -0.2f * innerRadius + barWidth);

    // ����б��
    // (Ϊ�򻯣�б����һ�����Ƶľ��α�ʾ���Ӿ��ϲ����С)
    add3DRect(0.2f * innerRadius, 0.4f * innerRadius - barWidth, 0.4f * innerRadius + barWidth, 0.2f * innerRadius);
    add3DRect(-0.4f * innerRadius - barWidth, 0.2f * innerRadius, -0.2f * innerRadius, 0.4f * innerRadius + barWidth);
    add3DRect(-0.4f * innerRadius - barWidth, -0.2f * innerRadius, -0.2f * innerRadius, -0.4f * innerRadius - barWidth);
    add3DRect(0.2f * innerRadius, -0.4f * innerRadius - barWidth, 0.4f * innerRadius + barWidth, -0.2f * innerRadius);

    // ��ÿ�����������������Ļ�һ��һ����ֱ��
    add3DRect(-0.85f * innerRadius, 0.6f * innerRadius - 0.5f * barWidth, 0.85f * innerRadius, 0.6f * innerRadius + 0.5f * barWidth); // ��
    add3DRect(-0.85f * innerRadius, -0.6f * innerRadius - 0.5f * barWidth, 0.85f * innerRadius, -0.6f * innerRadius + 0.5f * barWidth); // ��
    add3DRect(-0.6f * innerRadius - 0.5f * barWidth, -0.85f * innerRadius, -0.6f * innerRadius + 0.5f * barWidth, 0.85f * innerRadius); // ��
    add3DRect(0.6f * innerRadius - 0.5f * barWidth, -0.85f * innerRadius, 0.6f * innerRadius + 0.5f * barWidth, 0.85f * innerRadius); // ��

    // ��ÿ��б�ߵ��е㿪ʼ, ������һ��ֱ��
    // (Ϊ�򻯣�ͬ���ý��Ƶľ��α�ʾ)
    add3DRect(-0.3f * innerRadius - 0.5f * barWidth, 0.3f * innerRadius + 0.5f * barWidth, -0.3f * innerRadius + 0.5f * barWidth, 0.96f * innerRadius); // ����
    add3DRect(0.3f * innerRadius - 0.5f * barWidth, 0.3f * innerRadius + 0.5f * barWidth, 0.3f * innerRadius + 0.5f * barWidth, 0.96f * innerRadius); // ����
    add3DRect(-0.3f * innerRadius - 0.5f * barWidth, -0.96f * innerRadius, -0.3f * innerRadius + 0.5f * barWidth, -0.3f * innerRadius - 0.5f * barWidth); // ����
    add3DRect(0.3f * innerRadius - 0.5f * barWidth, -0.96f * innerRadius, 0.3f * innerRadius + 0.5f * barWidth, -0.3f * innerRadius - 0.5f * barWidth); // ����
    add3DRect(0.3f * innerRadius + 0.5f * barWidth, 0.3f * innerRadius - 0.5f * barWidth, 0.96f * innerRadius, 0.3f * innerRadius + 0.5f * barWidth); // ����
    add3DRect(0.3f * innerRadius + 0.5f * barWidth, -0.3f * innerRadius - 0.5f * barWidth, 0.96f * innerRadius, -0.3f * innerRadius + 0.5f * barWidth); // ����
    add3DRect(-0.96f * innerRadius, 0.3f * innerRadius - 0.5f * barWidth, -0.3f * innerRadius - 0.5f * barWidth, 0.3f * innerRadius + 0.5f * barWidth); // ����
    add3DRect(-0.96f * innerRadius, -0.3f * innerRadius - 0.5f * barWidth, -0.3f * innerRadius - 0.5f * barWidth, -0.3f * innerRadius + 0.5f * barWidth); // ����

    return vertices;
}

// ���ɴ�Բ�ο׶���ǽ�ڶ���
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

        // ��Ȧ���� (���ڱ�Ե)
        glm::vec3 inner1(holeRadius * cosf(angle1), holeRadius * sinf(angle1), 0.0f);
        glm::vec3 inner2(holeRadius * cosf(angle2), holeRadius * sinf(angle2), 0.0f);

        // ��Ȧ���� (ǽ�ڱ�Ե)
        // ʹ��һ���㹻�����Ӿ�����ȷ���ⶥ�㣬Ȼ����ü���ǽ��ʵ�ʱ߽�
        float outer_x1 = std::max(-halfW, std::min(halfW, inner1.x * 100)); // ���Դ�����Ͷ�䵽��Ե
        float outer_y1 = std::max(-halfH, std::min(halfH, inner1.y * 100));
        // �������ǽ�ǣ�ȷ������ȷ���ڽ���
        if (abs(outer_x1) == halfW && abs(outer_y1) > 0) outer_y1 = (inner1.y > 0) ? halfH : -halfH;
        if (abs(outer_y1) == halfH && abs(outer_x1) > 0) outer_x1 = (inner1.x > 0) ? halfW : -halfW;
        
        float outer_x2 = std::max(-halfW, std::min(halfW, inner2.x * 100));
        float outer_y2 = std::max(-halfH, std::min(halfH, inner2.y * 100));
        if (abs(outer_x2) == halfW && abs(outer_y2) > 0) outer_y2 = (inner2.y > 0) ? halfH : -halfH;
        if (abs(outer_y2) == halfH && abs(outer_x2) > 0) outer_x2 = (inner2.x > 0) ? halfW : -halfW;


        glm::vec3 outer1(outer_x1, outer_y1, 0.0f);
        glm::vec3 outer2(outer_x2, outer_y2, 0.0f);
        
        // ���ߣ����ں�ǽ��������Z��
        glm::vec3 normal(0.0f, 0.0f, 1.0f);

        // �����������ι���һ������
        // ������ 1
        vertices.insert(vertices.end(), {inner1.x, inner1.y, inner1.z, normal.x, normal.y, normal.z});
        vertices.insert(vertices.end(), {outer1.x, outer1.y, outer1.z, normal.x, normal.y, normal.z});
        vertices.insert(vertices.end(), {outer2.x, outer2.y, outer2.z, normal.x, normal.y, normal.z});

        // ������ 2
        vertices.insert(vertices.end(), {inner1.x, inner1.y, inner1.z, normal.x, normal.y, normal.z});
        vertices.insert(vertices.end(), {outer2.x, outer2.y, outer2.z, normal.x, normal.y, normal.z});
        vertices.insert(vertices.end(), {inner2.x, inner2.y, inner2.z, normal.x, normal.y, normal.z});
    }

    return vertices;
}