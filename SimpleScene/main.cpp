#include <glad/glad.h>
#include <GLFW/glfw3.h>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "shader.h"
#include "camera.h"

#include "model.h"
#include "sandbox_mesh.h"
#include "particle_system.h"
#include "lightning.h"
#include "arrow_system.h"

#include <iostream>
#include <vector>
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

bool lampOn = false; // 台灯是否打开
glm::vec3 lampModelWorldPos(0.0f); // 台灯在世界坐标系中的位置
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods); // 鼠标点击回调函数
bool rayHitLamp(const glm::vec3& rayOrigin, const glm::vec3& rayDir); // 射线检测函数，判断是否击中台灯
bool isNearHorse(const glm::vec3& playerPos, const glm::vec3& horsePos, float threshold); // 检查玩家是否靠近马雕像
glm::vec3 clampToRoomBounds(const glm::vec3& position); // 限制位置在房间范围内
bool areHorsesAtZAxisEnd(); // 检查两个马是否都在z轴尽头（已弃用，改用isHorse1OnTile1和isHorse2OnTile2）
bool isHorse1OnTile1(); // 检查马1是否在地砖1范围内
bool isHorse2OnTile2(); // 检查马2是否在地砖2范围内

// 马雕像控制状态
bool isControllingHorse = false; // 是否正在控制马雕像
bool eKeyPressed = false; // E键状态
glm::vec3 horse1Position(0.0f); // 第一个马雕像的位置
glm::vec3 horse2Position(0.0f); // 第二个马雕像的位置
float horse1RotationY = 0.0f; // 第一个马雕像的Y轴旋转
float horse1RotationX = 0.0f; // 第一个马雕像的X轴旋转
float horse2RotationY = 0.0f; // 第二个马雕像的Y轴旋转
float horse2RotationX = 0.0f; // 第二个马雕像的X轴旋转
int controlledHorseIndex = 0; // 当前控制的马雕像索引（0或1）
const float INTERACTION_DISTANCE = 3.0f; // 交互距离阈值
glm::vec3 savedCameraPosition(0.0f); // 保存进入控制模式前的相机位置
float savedCameraYaw = 0.0f; // 保存进入控制模式前的相机Yaw
float savedCameraPitch = 0.0f; // 保存进入控制模式前的相机Pitch
glm::vec3 initialCameraPosition(0.0f, 0.0f, 11.0f); // 程序启动时的初始相机位置
float initialCameraYaw = -90.0f; // 程序启动时的初始相机Yaw
float initialCameraPitch = 0.0f; // 程序启动时的初始相机Pitch

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

// Bloom 相关变量
unsigned int hdrFBO, pingpongFBO[2], colorBuffers[2], pingpongColorBuffers[2];
unsigned int quadVAO, quadVBO;
Shader* bloomThresholdShader = nullptr;
Shader* bloomBlurShader = nullptr;
Shader* bloomFinalShader = nullptr;
int bloomAmount = 10;  // 模糊迭代次数

// 地板翘起相关变量
bool isFloorTileLifting = false;      // 地板是否正在翘起
float floorLiftProgress = 0.0f;       // 翘起进度 (0.0 到 1.0)
float floorLiftSpeed = 0.5f;          // 翘起速度
const float Z_AXIS_THRESHOLD = 3.0f; // z轴触发阈值（房间z范围是-3.5到3.5，-3.0接近尽头）
unsigned int liftedTileVAO, liftedTileVBO; // 翘起地板块的VAO/VBO
unsigned int liftedTileHoleVAO, liftedTileHoleVBO; // 无盖长方体（洞）的VAO/VBO
// 地砖触发相关变量（z轴尽头）
const float BRICK_TILE_SIZE = 1.5f;        // 地砖大小（1.5x1.5，确保比马的底座大）
const float BRICK_TILE_GAP = 0.5f;         // 两块地砖之间的间距
const float BRICK_TILE_Z = 3.0f;           // 地砖的z坐标（接近z轴尽头3.5）
const float BRICK_TILE_Y = -2.92f;         // 地砖的y坐标（与地板顶部对齐）
// 地砖1位置（左侧，对应马1）：x中心=-2.0
const float BRICK_TILE1_CENTER_X = -2.0f;
const float BRICK_TILE1_MIN_X = BRICK_TILE1_CENTER_X - BRICK_TILE_SIZE * 0.5f;  // -3.0
const float BRICK_TILE1_MAX_X = BRICK_TILE1_CENTER_X + BRICK_TILE_SIZE * 0.5f;  // -1.0
const float BRICK_TILE1_MIN_Z = BRICK_TILE_Z - BRICK_TILE_SIZE * 0.5f;          // 2.0
const float BRICK_TILE1_MAX_Z = BRICK_TILE_Z + BRICK_TILE_SIZE * 0.5f;          // 4.0
// 地砖2位置（右侧，对应马2）：x中心=2.0
const float BRICK_TILE2_CENTER_X = 2.0f;
const float BRICK_TILE2_MIN_X = BRICK_TILE2_CENTER_X - BRICK_TILE_SIZE * 0.5f;  // 1.0
const float BRICK_TILE2_MAX_X = BRICK_TILE2_CENTER_X + BRICK_TILE_SIZE * 0.5f;  // 3.0
const float BRICK_TILE2_MIN_Z = BRICK_TILE_Z - BRICK_TILE_SIZE * 0.5f;          // 2.0
const float BRICK_TILE2_MAX_Z = BRICK_TILE_Z + BRICK_TILE_SIZE * 0.5f;          // 4.0
unsigned int brickTile1VAO, brickTile1VBO; // 地砖1的VAO/VBO
unsigned int brickTile2VAO, brickTile2VBO; // 地砖2的VAO/VBO
bool isHorse1OnTile1(); // 检查马1是否在地砖1范围内
bool isHorse2OnTile2(); // 检查马2是否在地砖2范围内

// 灵珠拾取相关变量
bool orbPicked = false;               // 灵珠是否已被拾取
bool showOrbPickupMessage = false;    // 是否显示拾取消息
float orbPickupMessageTime = 0.0f;    // 拾取消息显示时间
const float ORB_PICKUP_MESSAGE_DURATION = 3.0f; // 拾取消息显示持续时间（秒）
// 灵珠位置（全局变量，用于processInput访问）
float g_tileWorldCenterX = 0.0f;
float g_holeCenterY = 0.0f;
float g_tileWorldCenterZ = 0.0f;

// 书柜按钮相关变量（已注释，改用马的旋转触发）
// glm::vec3 shelfPosition(3.95f, -2.90f, 2.0f); // 书柜位置（可动态移动）
glm::vec3 shelfPosition(3.55f, -1.15f, 2.0f); // 书柜位置（可动态移动）
// glm::vec3 buttonPosition(3.94f, -1.2f, 3.0f); // 按钮位置（已注释）
// bool buttonPressed = false;           // 按钮是否已按下（已注释）
// bool showButtonEPrompt = false;       // 是否显示按钮的E提示（已注释）
// glm::vec3 buttonELetterPos(0.0f);    // 按钮E提示的位置（已注释）
const float SHELF_MOVE_DISTANCE = 2.5f; // 书柜向z轴负方向平移的距离
bool shelfMoving = false;             // 书柜是否正在移动
float shelfMoveProgress = 0.0f;       // 书柜移动进度（0.0到1.0）
const float SHELF_MOVE_SPEED = 1.0f; // 书柜移动速度
// 砖墙旋转相关变量
bool brickSquareRotating = false;     // 砖墙是否正在旋转
bool brickSquareRotatingBack = false; // 砖墙是否正在反向旋转
float brickSquareRotationProgress = 0.0f; // 砖墙旋转进度（0.0到1.0）
float brickSquareRestTime = 0.0f;     // 砖墙旋转完成后的静止时间
const float BRICK_SQUARE_ROTATION_SPEED = 0.5f; // 砖墙旋转速度（每秒）
const float BRICK_SQUARE_ROTATION_ANGLE = 90.0f; // 砖墙旋转角度（度）
const float BRICK_SQUARE_REST_DURATION = 2.0f; // 砖墙旋转完成后的静止时间（秒）
const float SHELF_INITIAL_Z = 2.0f;   // 书柜初始Z位置
bool shelfMovingBack = false;         // 书柜是否正在反向移动
// 马的旋转触发相关变量
float horse1InitialRotationY = 90.0f;  // 马1的初始Y轴旋转角度
float horse2InitialRotationY = -90.0f; // 马2的初始Y轴旋转角度
float horse1AccumulatedRotation = 0.0f; // 马1累计转过的度数（从初始角度开始）
float horse2AccumulatedRotation = 0.0f; // 马2累计转过的度数（从初始角度开始）
const float HORSE_ROTATION_THRESHOLD = 45.0f; // 触发动画所需的旋转角度阈值（度）
bool isAnimationSequenceActive = false; // 动画序列是否正在执行（书柜移动、砖墙旋转等）

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
    glfwSetMouseButtonCallback(window, mouse_button_callback); // 注册鼠标点击回调函数

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
    // 初始化雷电 Shader
    Shader* lightningShader = nullptr;
    try {
        lightningShader = new Shader("lightning.vs", "lightning.fs");
    } catch (...) {
        std::cout << "ERROR: Failed to load lightning shader!" << std::endl;
    }
    // Bloom 着色器
    try {
        bloomThresholdShader = new Shader("bloom_blur.vs", "bloom_threshold.fs");
        bloomBlurShader = new Shader("bloom_blur.vs", "bloom_blur.fs");
        bloomFinalShader = new Shader("bloom_blur.vs", "bloom_final.fs");
    } catch (...) {
        std::cout << "ERROR: Failed to load bloom shaders!" << std::endl;
    }
    // 文字显示着色器（屏幕空间，使用纹理显示中文）
    Shader* textDisplayShader = nullptr;
    try {
        textDisplayShader = new Shader("text_display_texture.vs", "text_display_texture.fs");
    } catch (...) {
        std::cout << "ERROR: Failed to load text display shader!" << std::endl;
    }
    // Billboard着色器（世界空间）
    Shader* billboardShader = nullptr;
    try {
        billboardShader = new Shader("billboard.vs", "billboard.fs");
    } catch (...) {
        std::cout << "ERROR: Failed to load billboard shader!" << std::endl;
    }
    // 灵珠着色器
    Shader* orbShader = nullptr;
    try {
        orbShader = new Shader("orb.vs", "orb.fs");
    } catch (...) {
        std::cout << "ERROR: Failed to load orb shader!" << std::endl;
    }

    // --- 加载模型 ---
    Model ourModel("resource/model/table3.obj");
    Model lampModel("resource/lamp/lamp1.obj");
    // 加载马雕像模型（尝试多种可能的文件格式）
    Model* horseModel = nullptr;
    std::vector<std::string> possiblePaths = {
        "resource/horse_statue_01_4k.blend/horse_statue_01_4k.obj",
        "resource/horse_statue_01_4k.blend/horse_statue_01_4k.fbx",
        "resource/horse_statue_01_4k.blend/horse_statue_01_4k.blend"
    };
    for (const auto& path : possiblePaths) {
        try {
            horseModel = new Model(path);
            // 检查模型是否成功加载（通过检查是否有网格）
            if (horseModel && horseModel->meshes.size() > 0) {
                std::cout << "Loaded horse statue model successfully from: " << path << std::endl;
                break;
            } else {
                delete horseModel;
                horseModel = nullptr;
            }
        } catch (...) {
            if (horseModel) {
                delete horseModel;
                horseModel = nullptr;
            }
        }
    }
    if (!horseModel) {
        std::cout << "Warning: Failed to load horse statue model. Please export the .blend file as .obj or .fbx format and place it in the resource/horse_statue_01_4k.blend/ folder." << std::endl;
    }

    // 加载书架模型
    Model* shelfModel = nullptr;
    try {
        shelfModel = new Model("resource/shelf/bookcase1.obj");
        // 检查模型是否成功加载（通过检查是否有网格）
        if (shelfModel && shelfModel->meshes.size() > 0) {
            std::cout << "Loaded shelf model successfully from: resource/shelf/bookcase1.obj" << std::endl;
        } else {
            delete shelfModel;
            shelfModel = nullptr;
            std::cout << "Warning: Failed to load shelf model from resource/shelf/bookcase1.obj" << std::endl;
        }
    } catch (...) {
        if (shelfModel) {
            delete shelfModel;
            shelfModel = nullptr;
        }
        std::cout << "Warning: Failed to load shelf model from resource/shelf/bookcase1.obj" << std::endl;
    }

    // 加载箭头模型
    Model* arrowModel = nullptr;
    try {
        arrowModel = new Model("resource/arrow/arrow1.obj");
        // 检查模型是否成功加载（通过检查是否有网格）
        if (arrowModel && arrowModel->meshes.size() > 0) {
            std::cout << "Loaded arrow model successfully from: resource/arrow/arrow1.obj" << std::endl;
        } else {
            delete arrowModel;
            arrowModel = nullptr;
            std::cout << "Warning: Failed to load arrow model from resource/arrow/arrow1.obj" << std::endl;
        }
    } catch (...) {
        if (arrowModel) {
            delete arrowModel;
            arrowModel = nullptr;
        }
        std::cout << "Warning: Failed to load arrow model from resource/arrow/arrow1.obj" << std::endl;
    }

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
    sandbox_heightmap.setupMesh(sandboxShader); 

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

    // --- 加载地砖纹理 ---
    unsigned int floorTexture;
    glGenTextures(1, &floorTexture);
    glBindTexture(GL_TEXTURE_2D, floorTexture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    int floorWidth, floorHeight, floorNrChannels;
    unsigned char *floorData = stbi_load("resource/plank_flooring_04_4k.blend/textures/plank_flooring_04_diff_4k.jpg", &floorWidth, &floorHeight, &floorNrChannels, 0);
    if (floorData)
    {
        GLenum format = GL_RGB;
        if (floorNrChannels == 1)
            format = GL_RED;
        else if (floorNrChannels == 3)
            format = GL_RGB;
        else if (floorNrChannels == 4)
            format = GL_RGBA;
        
        glTexImage2D(GL_TEXTURE_2D, 0, format, floorWidth, floorHeight, 0, format, GL_UNSIGNED_BYTE, floorData);
        glGenerateMipmap(GL_TEXTURE_2D);
        std::cout << "Loaded floor texture successfully" << std::endl;
    }
    else
    {
        std::cout << "Failed to load floor texture" << std::endl;
    }
    stbi_image_free(floorData);

    // --- 加载木纹纹理（用于天花板和墙壁） ---
    unsigned int woodTexture;
    glGenTextures(1, &woodTexture);
    glBindTexture(GL_TEXTURE_2D, woodTexture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    int woodWidth, woodHeight, woodNrChannels;
    unsigned char *woodData = stbi_load("resource/wood_cabinet_worn_long_4k.blend/textures/wood_cabinet_worn_long_diff_4k.jpg", &woodWidth, &woodHeight, &woodNrChannels, 0);
    if (woodData)
    {
        GLenum format = GL_RGB;
        if (woodNrChannels == 1)
            format = GL_RED;
        else if (woodNrChannels == 3)
            format = GL_RGB;
        else if (woodNrChannels == 4)
            format = GL_RGBA;
        
        glTexImage2D(GL_TEXTURE_2D, 0, format, woodWidth, woodHeight, 0, format, GL_UNSIGNED_BYTE, woodData);
        glGenerateMipmap(GL_TEXTURE_2D);
        std::cout << "Loaded wood texture successfully" << std::endl;
    }
    else
    {
        std::cout << "Failed to load wood texture" << std::endl;
    }
    stbi_image_free(woodData);

    // --- 加载砖墙纹理（用于书柜后面的墙） ---
    unsigned int brickTexture;
    glGenTextures(1, &brickTexture);
    glBindTexture(GL_TEXTURE_2D, brickTexture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    int brickWidth, brickHeight, brickNrChannels;
    unsigned char *brickData = stbi_load("resource/pavement_04_4k.blend/textures/pavement_04_diff_4k.jpg", &brickWidth, &brickHeight, &brickNrChannels, 0);
    if (brickData)
    {
        GLenum format = GL_RGB;
        if (brickNrChannels == 1)
            format = GL_RED;
        else if (brickNrChannels == 3)
            format = GL_RGB;
        else if (brickNrChannels == 4)
            format = GL_RGBA;
        
        glTexImage2D(GL_TEXTURE_2D, 0, format, brickWidth, brickHeight, 0, format, GL_UNSIGNED_BYTE, brickData);
        glGenerateMipmap(GL_TEXTURE_2D);
        std::cout << "Loaded brick texture successfully" << std::endl;
    }
    else
    {
        std::cout << "Failed to load brick texture" << std::endl;
    }
    stbi_image_free(brickData);

    // --- 加载砖墙纹理（用于z轴尽头的地砖） ---
    unsigned int brickWallTexture;
    glGenTextures(1, &brickWallTexture);
    glBindTexture(GL_TEXTURE_2D, brickWallTexture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    int brickWallWidth, brickWallHeight, brickWallNrChannels;
    unsigned char *brickWallData = stbi_load("resource/brick_wall_10_4k.blend/textures/brick_wall_10_diff_4k.jpg", &brickWallWidth, &brickWallHeight, &brickWallNrChannels, 0);
    if (brickWallData)
    {
        GLenum format = GL_RGB;
        if (brickWallNrChannels == 1)
            format = GL_RED;
        else if (brickWallNrChannels == 3)
            format = GL_RGB;
        else if (brickWallNrChannels == 4)
            format = GL_RGBA;
        
        glTexImage2D(GL_TEXTURE_2D, 0, format, brickWallWidth, brickWallHeight, 0, format, GL_UNSIGNED_BYTE, brickWallData);
        glGenerateMipmap(GL_TEXTURE_2D);
        std::cout << "Loaded brick wall texture successfully" << std::endl;
    }
    else
    {
        std::cout << "Failed to load brick wall texture" << std::endl;
    }
    stbi_image_free(brickWallData);

    // --- 加载中文消息纹理（用于显示"已拾取灵珠！"） ---
    unsigned int messageTexture;
    glGenTextures(1, &messageTexture);
    glBindTexture(GL_TEXTURE_2D, messageTexture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    int messageWidth, messageHeight, messageNrChannels;
    // 注意：需要创建一个包含"已拾取灵珠！"文字的PNG图片，放在resource/textures/目录下
    // 图片应该是透明背景，白色或黄色文字，建议尺寸：600x150像素
    unsigned char *messageData = stbi_load("resource/textures/orb_pickup_message.png", &messageWidth, &messageHeight, &messageNrChannels, 4);
    if (messageData)
    {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, messageWidth, messageHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, messageData);
        glGenerateMipmap(GL_TEXTURE_2D);
        std::cout << "Loaded message texture successfully" << std::endl;
    }
    else
    {
        std::cout << "Failed to load message texture: resource/textures/orb_pickup_message.png" << std::endl;
        std::cout << "Please create a PNG image with Chinese text '已拾取灵珠！' (transparent background, white/yellow text)" << std::endl;
    }
    stbi_image_free(messageData);

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
    // 加载雷电纹理
    unsigned int lightningTexture;
    glGenTextures(1, &lightningTexture);
    glBindTexture(GL_TEXTURE_2D, lightningTexture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    int lw, lh, lnr;
    // 最后一个参数传 4，强制加载为 RGBA 格式
    unsigned char *ldata = stbi_load("resource/textures/lightning.png", &lw, &lh, &lnr, 4);
    if (ldata) {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, lw, lh, 0, GL_RGBA, GL_UNSIGNED_BYTE, ldata);
        glGenerateMipmap(GL_TEXTURE_2D);
    } else {
        std::cout << "Failed to load lightning texture" << std::endl;
    }
    stbi_image_free(ldata);

    // 创建 Lightning 对象
    Lightning* lightning = nullptr;
    if (lightningShader) {
        lightning = new Lightning(lightningShader, lightningTexture);
    }

    // 创建 ArrowSystem 对象（砖墙处）
    ArrowSystem* arrowSystem = nullptr;
    if (arrowModel) {
        // 砖墙位置：x=3.94, y=-2.20, z=2.2
        // 箭头从砖墙中心发射，朝向房间内部（负X方向）
        glm::vec3 arrowSpawnPos(3.94f, -2.20f, 2.2f);
        arrowSystem = new ArrowSystem(arrowModel, &modelShader, arrowSpawnPos);
        // 设置发射方向（朝向房间内部，即负X方向）
        arrowSystem->SetSpawnDirection(glm::normalize(glm::vec3(-1.0f, 0.0f, 0.0f)));
        std::cout << "Arrow system initialized" << std::endl;
    }

    // 创建 ArrowSystem 对象（窗外）
    ArrowSystem* windowArrowSystem = nullptr;
    if (arrowModel) {
        // 窗户位置：后墙在z=-4.0，窗户在后墙前一点
        // 箭头从窗外发射，位置在窗户外面（z=-5.0），朝向房间内部（z轴正方向）
        glm::vec3 windowArrowSpawnPos(0.0f, 0.0f, -5.0f);
        windowArrowSystem = new ArrowSystem(arrowModel, &modelShader, windowArrowSpawnPos);
        // 设置发射方向（朝向房间内部，即z轴正方向）
        windowArrowSystem->SetSpawnDirection(glm::normalize(glm::vec3(0.0f, 0.0f, 1.0f)));
        
        // 设置窗外箭头的旋转角度（使其朝向z轴正方向）
        // 如果箭头模型默认朝向y轴负方向，要转到z轴正方向：
        // 先绕x轴旋转90度：y负 -> z正
        windowArrowSystem->SetRotationOffset(90.0f, 0.0f, 0.0f);  // 绕X轴旋转90度
        
        std::cout << "Window arrow system initialized" << std::endl;
    }

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

    // --- 创建带纹理坐标的地板顶点数据（有厚度） ---
    // 地板顶点：位置(3) + 法线(3) + 纹理坐标(2) = 8个float
    // 纹理坐标设为2.0，实现2x2重复，每个砖块更大
    // 地板厚度为0.15，从y=-0.5到y=-0.35（相对于地板中心）
    // 底部在y=-2.95，顶部在y=-2.80（通过translate和scale计算得出）
    float floorVertices[] = {
        // 顶部面（向上）
        -0.5f, -0.35f, -0.5f,  0.0f,  1.0f, 0.0f,  0.0f, 0.0f,
         0.5f, -0.35f, -0.5f,  0.0f,  1.0f, 0.0f,  2.0f, 0.0f,
         0.5f, -0.35f,  0.5f,  0.0f,  1.0f, 0.0f,  2.0f, 2.0f,
         0.5f, -0.35f,  0.5f,  0.0f,  1.0f, 0.0f,  2.0f, 2.0f,
        -0.5f, -0.35f,  0.5f,  0.0f,  1.0f, 0.0f,  0.0f, 2.0f,
        -0.5f, -0.35f, -0.5f,  0.0f,  1.0f, 0.0f,  0.0f, 0.0f,
        
        // 底部面（向下）
        -0.5f, -0.5f, -0.5f,  0.0f, -1.0f, 0.0f,  0.0f, 0.0f,
         0.5f, -0.5f, -0.5f,  0.0f, -1.0f, 0.0f,  2.0f, 0.0f,
         0.5f, -0.5f,  0.5f,  0.0f, -1.0f, 0.0f,  2.0f, 2.0f,
         0.5f, -0.5f,  0.5f,  0.0f, -1.0f, 0.0f,  2.0f, 2.0f,
        -0.5f, -0.5f,  0.5f,  0.0f, -1.0f, 0.0f,  0.0f, 2.0f,
        -0.5f, -0.5f, -0.5f,  0.0f, -1.0f, 0.0f,  0.0f, 0.0f,
        
        // 前面（+Z方向）
        -0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f, 0.0f,
         0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  2.0f, 0.0f,
         0.5f, -0.35f,  0.5f,  0.0f,  0.0f,  1.0f,  2.0f, 0.15f,
         0.5f, -0.35f,  0.5f,  0.0f,  0.0f,  1.0f,  2.0f, 0.15f,
        -0.5f, -0.35f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f, 0.15f,
        -0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f, 0.0f,
        
        // 后面（-Z方向）
        -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f, 0.0f,
         0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  2.0f, 0.0f,
         0.5f, -0.35f, -0.5f,  0.0f,  0.0f, -1.0f,  2.0f, 0.15f,
         0.5f, -0.35f, -0.5f,  0.0f,  0.0f, -1.0f,  2.0f, 0.15f,
        -0.5f, -0.35f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f, 0.15f,
        -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f, 0.0f,
        
        // 右面（+X方向）
         0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  0.0f, 0.0f,
         0.5f, -0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  2.0f, 0.0f,
         0.5f, -0.35f,  0.5f,  1.0f,  0.0f,  0.0f,  2.0f, 0.15f,
         0.5f, -0.35f,  0.5f,  1.0f,  0.0f,  0.0f,  2.0f, 0.15f,
         0.5f, -0.35f, -0.5f,  1.0f,  0.0f,  0.0f,  0.0f, 0.15f,
         0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  0.0f, 0.0f,
        
        // 左面（-X方向）
        -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  0.0f, 0.0f,
        -0.5f, -0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  2.0f, 0.0f,
        -0.5f, -0.35f,  0.5f, -1.0f,  0.0f,  0.0f,  2.0f, 0.15f,
        -0.5f, -0.35f,  0.5f, -1.0f,  0.0f,  0.0f,  2.0f, 0.15f,
        -0.5f, -0.35f, -0.5f, -1.0f,  0.0f,  0.0f,  0.0f, 0.15f,
        -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  0.0f, 0.0f
    };
    
    unsigned int floorVAO, floorVBO;
    glGenVertexArrays(1, &floorVAO);
    glGenBuffers(1, &floorVBO);
    glBindVertexArray(floorVAO);
    glBindBuffer(GL_ARRAY_BUFFER, floorVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(floorVertices), floorVertices, GL_STATIC_DRAW);
    // 位置属性
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    // 法线属性
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    // 纹理坐标属性
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);
    glBindVertexArray(0);

    // --- 创建z轴尽头的地砖几何体（两块地砖，用于触发检测） ---
    // 地砖大小：2.0 x 2.0，厚度0.15（与地板相同）
    // 地砖1：左侧，中心x=-2.0, z=3.0
    // 地砖2：右侧，中心x=2.0, z=3.0
    // 两块地砖之间有间距（0.5单位）
    float brickTileHalfSize = BRICK_TILE_SIZE * 0.5f;  // 1.0
    float brickTileHalfHeight = 0.075f;  // 0.15/2
    
    // 地砖顶点数据（与地板相同结构：位置(3) + 法线(3) + 纹理坐标(2) = 8个float）
    float brickTileVertices[] = {
        // 顶部面（向上）
        -brickTileHalfSize, brickTileHalfHeight, -brickTileHalfSize,  0.0f,  1.0f, 0.0f,  0.0f, 0.0f,
         brickTileHalfSize, brickTileHalfHeight, -brickTileHalfSize,  0.0f,  1.0f, 0.0f,  1.0f, 0.0f,
         brickTileHalfSize, brickTileHalfHeight,  brickTileHalfSize,  0.0f,  1.0f, 0.0f,  1.0f, 1.0f,
         brickTileHalfSize, brickTileHalfHeight,  brickTileHalfSize,  0.0f,  1.0f, 0.0f,  1.0f, 1.0f,
        -brickTileHalfSize, brickTileHalfHeight,  brickTileHalfSize,  0.0f,  1.0f, 0.0f,  0.0f, 1.0f,
        -brickTileHalfSize, brickTileHalfHeight, -brickTileHalfSize,  0.0f,  1.0f, 0.0f,  0.0f, 0.0f,
        
        // 底部面（向下）
        -brickTileHalfSize, -brickTileHalfHeight, -brickTileHalfSize,  0.0f, -1.0f, 0.0f,  0.0f, 0.0f,
         brickTileHalfSize, -brickTileHalfHeight, -brickTileHalfSize,  0.0f, -1.0f, 0.0f,  1.0f, 0.0f,
         brickTileHalfSize, -brickTileHalfHeight,  brickTileHalfSize,  0.0f, -1.0f, 0.0f,  1.0f, 1.0f,
         brickTileHalfSize, -brickTileHalfHeight,  brickTileHalfSize,  0.0f, -1.0f, 0.0f,  1.0f, 1.0f,
        -brickTileHalfSize, -brickTileHalfHeight,  brickTileHalfSize,  0.0f, -1.0f, 0.0f,  0.0f, 1.0f,
        -brickTileHalfSize, -brickTileHalfHeight, -brickTileHalfSize,  0.0f, -1.0f, 0.0f,  0.0f, 0.0f,
        
        // 前面（+Z方向）
        -brickTileHalfSize, -brickTileHalfHeight, brickTileHalfSize,  0.0f,  0.0f,  1.0f,  0.0f, 0.0f,
         brickTileHalfSize, -brickTileHalfHeight, brickTileHalfSize,  0.0f,  0.0f,  1.0f,  1.0f, 0.0f,
         brickTileHalfSize,  brickTileHalfHeight, brickTileHalfSize,  0.0f,  0.0f,  1.0f,  1.0f, 1.0f,
         brickTileHalfSize,  brickTileHalfHeight, brickTileHalfSize,  0.0f,  0.0f,  1.0f,  1.0f, 1.0f,
        -brickTileHalfSize,  brickTileHalfHeight, brickTileHalfSize,  0.0f,  0.0f,  1.0f,  0.0f, 1.0f,
        -brickTileHalfSize, -brickTileHalfHeight, brickTileHalfSize,  0.0f,  0.0f,  1.0f,  0.0f, 0.0f,
        
        // 后面（-Z方向）
        -brickTileHalfSize, -brickTileHalfHeight, -brickTileHalfSize,  0.0f,  0.0f, -1.0f,  0.0f, 0.0f,
         brickTileHalfSize, -brickTileHalfHeight, -brickTileHalfSize,  0.0f,  0.0f, -1.0f,  1.0f, 0.0f,
         brickTileHalfSize,  brickTileHalfHeight, -brickTileHalfSize,  0.0f,  0.0f, -1.0f,  1.0f, 1.0f,
         brickTileHalfSize,  brickTileHalfHeight, -brickTileHalfSize,  0.0f,  0.0f, -1.0f,  1.0f, 1.0f,
        -brickTileHalfSize,  brickTileHalfHeight, -brickTileHalfSize,  0.0f,  0.0f, -1.0f,  0.0f, 1.0f,
        -brickTileHalfSize, -brickTileHalfHeight, -brickTileHalfSize,  0.0f,  0.0f, -1.0f,  0.0f, 0.0f,
        
        // 右面（+X方向）
         brickTileHalfSize, -brickTileHalfHeight, -brickTileHalfSize,  1.0f,  0.0f,  0.0f,  0.0f, 0.0f,
         brickTileHalfSize, -brickTileHalfHeight,  brickTileHalfSize,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f,
         brickTileHalfSize,  brickTileHalfHeight,  brickTileHalfSize,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f,
         brickTileHalfSize,  brickTileHalfHeight,  brickTileHalfSize,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f,
         brickTileHalfSize,  brickTileHalfHeight, -brickTileHalfSize,  1.0f,  0.0f,  0.0f,  0.0f, 1.0f,
         brickTileHalfSize, -brickTileHalfHeight, -brickTileHalfSize,  1.0f,  0.0f,  0.0f,  0.0f, 0.0f,
        
        // 左面（-X方向）
        -brickTileHalfSize, -brickTileHalfHeight, -brickTileHalfSize, -1.0f,  0.0f,  0.0f,  0.0f, 0.0f,
        -brickTileHalfSize, -brickTileHalfHeight,  brickTileHalfSize, -1.0f,  0.0f,  0.0f,  1.0f, 0.0f,
        -brickTileHalfSize,  brickTileHalfHeight,  brickTileHalfSize, -1.0f,  0.0f,  0.0f,  1.0f, 1.0f,
        -brickTileHalfSize,  brickTileHalfHeight,  brickTileHalfSize, -1.0f,  0.0f,  0.0f,  1.0f, 1.0f,
        -brickTileHalfSize,  brickTileHalfHeight, -brickTileHalfSize, -1.0f,  0.0f,  0.0f,  0.0f, 1.0f,
        -brickTileHalfSize, -brickTileHalfHeight, -brickTileHalfSize, -1.0f,  0.0f,  0.0f,  0.0f, 0.0f
    };
    
    // 创建地砖1的VAO/VBO
    glGenVertexArrays(1, &brickTile1VAO);
    glGenBuffers(1, &brickTile1VBO);
    glBindVertexArray(brickTile1VAO);
    glBindBuffer(GL_ARRAY_BUFFER, brickTile1VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(brickTileVertices), brickTileVertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);
    glBindVertexArray(0);
    
    // 创建地砖2的VAO/VBO（使用相同的数据）
    glGenVertexArrays(1, &brickTile2VAO);
    glGenBuffers(1, &brickTile2VBO);
    glBindVertexArray(brickTile2VAO);
    glBindBuffer(GL_ARRAY_BUFFER, brickTile2VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(brickTileVertices), brickTileVertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);
    glBindVertexArray(0);

    // --- 创建翘起地板块的几何体（房间偏右位置的一小块） ---
    // 这块地板位于世界坐标：x: 1.5 到 3.0, z: -1.5 到 0.5
    // 为了便于变换，将顶点定义为中心在(0,0,0)的局部坐标系
    // 尺寸：宽度=1.5（x方向），深度=2.0（z方向），高度=0.15（y方向）
    // 所以局部坐标范围：x: [-0.75, 0.75], z: [-1.0, 1.0], y: [-0.075, 0.075]
    float tileHalfWidth = 0.75f;   // 1.5/2
    float tileHalfDepth = 1.0f;    // 2.0/2
    float tileHalfHeight = 0.075f; // 0.15/2
    
    // 世界坐标中心位置（原始投影位置）
    float tileProjectionCenterX = 2.25f;  // (1.5 + 3.0) / 2 - 原始投影位置的中心X
    float tileProjectionCenterZ = -0.5f;  // (-1.5 + 0.5) / 2 - 原始投影位置的中心Z
    
    // 翘起地板块向右平移后的位置（同时也是排除区域的中心）
    float tileWorldCenterX = tileProjectionCenterX + 0.5f;  // 向右平移0.5个单位 = 2.75
    float tileWorldCenterZ = tileProjectionCenterZ - 1.0f;  // Z坐标向负方向移动1.0个单位 = -1.5
    // 排除区域的中心坐标（用于翘起地板块、无盖长方体和灵珠的位置）
    // 排除区域：X: 2.0 到 3.5, Z: -2.5 到 -0.5（整体向z轴负方向移动1.0个单位）
    // 中心：(2.75, -1.5) - 与翘起地板块位置一致
    float excludedRegionCenterX = (2.0f + 3.5f) * 0.5f;  // 2.75（与tileWorldCenterX一致）
    float excludedRegionCenterZ = (-2.5f + (-0.5f)) * 0.5f; // -1.5（与tileWorldCenterZ一致）
    // 保存到全局变量，供翘起地板块、无盖长方体、灵珠和processInput使用（确保位置一致）
    g_tileWorldCenterX = excludedRegionCenterX;  // 2.75
    g_tileWorldCenterZ = excludedRegionCenterZ;  // -1.5
    // 主地板：translate(0, -2.50, 0) + scale(8.0, 1.0, 8.0)
    // 主地板局部坐标：y从-0.5到-0.35（相对于地板中心）
    // 主地板顶部世界坐标：-2.50 + (-0.35) × 1.0 = -2.85
    // 翘起地板块的顶部应该与主地板顶部对齐，所以地板块中心Y = -2.85 - 0.075 = -2.925
    float tileWorldCenterY = -2.925f; // 地板块中心Y坐标（使顶部与主地板顶部-2.85对齐）
    
    // 定义顶点（中心在原点，范围：x:[-0.75,0.75], z:[-1.0,1.0], y:[-0.075,0.075]）
    // 纹理坐标缩小范围以让纹理显示得更大（0.0到0.5，纹理会显示为原来的2倍大小）
    float textureScale = 0.5f; // 纹理缩放因子，值越小纹理越大
    float liftedTileVertices[] = {
        // 顶部面（向上）
        -tileHalfWidth, tileHalfHeight, -tileHalfDepth,  0.0f,  1.0f, 0.0f,  0.0f, 0.0f,
         tileHalfWidth, tileHalfHeight, -tileHalfDepth,  0.0f,  1.0f, 0.0f,  textureScale, 0.0f,
         tileHalfWidth, tileHalfHeight,  tileHalfDepth,  0.0f,  1.0f, 0.0f,  textureScale, textureScale,
         tileHalfWidth, tileHalfHeight,  tileHalfDepth,  0.0f,  1.0f, 0.0f,  textureScale, textureScale,
        -tileHalfWidth, tileHalfHeight,  tileHalfDepth,  0.0f,  1.0f, 0.0f,  0.0f, textureScale,
        -tileHalfWidth, tileHalfHeight, -tileHalfDepth,  0.0f,  1.0f, 0.0f,  0.0f, 0.0f,
        
        // 底部面（向下）
        -tileHalfWidth, -tileHalfHeight, -tileHalfDepth,  0.0f, -1.0f, 0.0f,  0.0f, 0.0f,
         tileHalfWidth, -tileHalfHeight, -tileHalfDepth,  0.0f, -1.0f, 0.0f,  textureScale, 0.0f,
         tileHalfWidth, -tileHalfHeight,  tileHalfDepth,  0.0f, -1.0f, 0.0f,  textureScale, textureScale,
         tileHalfWidth, -tileHalfHeight,  tileHalfDepth,  0.0f, -1.0f, 0.0f,  textureScale, textureScale,
        -tileHalfWidth, -tileHalfHeight,  tileHalfDepth,  0.0f, -1.0f, 0.0f,  0.0f, textureScale,
        -tileHalfWidth, -tileHalfHeight, -tileHalfDepth,  0.0f, -1.0f, 0.0f,  0.0f, 0.0f,
        
        // 前面（+Z方向）
        -tileHalfWidth, -tileHalfHeight, tileHalfDepth,  0.0f,  0.0f,  1.0f,  0.0f, 0.0f,
         tileHalfWidth, -tileHalfHeight, tileHalfDepth,  0.0f,  0.0f,  1.0f,  1.0f, 0.0f,
         tileHalfWidth,  tileHalfHeight, tileHalfDepth,  0.0f,  0.0f,  1.0f,  1.0f, 1.0f,
         tileHalfWidth,  tileHalfHeight, tileHalfDepth,  0.0f,  0.0f,  1.0f,  1.0f, 1.0f,
        -tileHalfWidth,  tileHalfHeight, tileHalfDepth,  0.0f,  0.0f,  1.0f,  0.0f, 1.0f,
        -tileHalfWidth, -tileHalfHeight, tileHalfDepth,  0.0f,  0.0f,  1.0f,  0.0f, 0.0f,
        
        // 后面（-Z方向）- 这是旋转轴所在的面
        -tileHalfWidth, -tileHalfHeight, -tileHalfDepth,  0.0f,  0.0f, -1.0f,  0.0f, 0.0f,
         tileHalfWidth, -tileHalfHeight, -tileHalfDepth,  0.0f,  0.0f, -1.0f,  1.0f, 0.0f,
         tileHalfWidth,  tileHalfHeight, -tileHalfDepth,  0.0f,  0.0f, -1.0f,  1.0f, 1.0f,
         tileHalfWidth,  tileHalfHeight, -tileHalfDepth,  0.0f,  0.0f, -1.0f,  1.0f, 1.0f,
        -tileHalfWidth,  tileHalfHeight, -tileHalfDepth,  0.0f,  0.0f, -1.0f,  0.0f, 1.0f,
        -tileHalfWidth, -tileHalfHeight, -tileHalfDepth,  0.0f,  0.0f, -1.0f,  0.0f, 0.0f,
        
        // 右面（+X方向）
         tileHalfWidth, -tileHalfHeight, -tileHalfDepth,  1.0f,  0.0f,  0.0f,  0.0f, 0.0f,
         tileHalfWidth, -tileHalfHeight,  tileHalfDepth,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f,
         tileHalfWidth,  tileHalfHeight,  tileHalfDepth,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f,
         tileHalfWidth,  tileHalfHeight,  tileHalfDepth,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f,
         tileHalfWidth,  tileHalfHeight, -tileHalfDepth,  1.0f,  0.0f,  0.0f,  0.0f, 1.0f,
         tileHalfWidth, -tileHalfHeight, -tileHalfDepth,  1.0f,  0.0f,  0.0f,  0.0f, 0.0f,
        
        // 左面（-X方向）
        -tileHalfWidth, -tileHalfHeight, -tileHalfDepth, -1.0f,  0.0f,  0.0f,  0.0f, 0.0f,
        -tileHalfWidth, -tileHalfHeight,  tileHalfDepth, -1.0f,  0.0f,  0.0f,  1.0f, 0.0f,
        -tileHalfWidth,  tileHalfHeight,  tileHalfDepth, -1.0f,  0.0f,  0.0f,  1.0f, 1.0f,
        -tileHalfWidth,  tileHalfHeight,  tileHalfDepth, -1.0f,  0.0f,  0.0f,  1.0f, 1.0f,
        -tileHalfWidth,  tileHalfHeight, -tileHalfDepth, -1.0f,  0.0f,  0.0f,  0.0f, 1.0f,
        -tileHalfWidth, -tileHalfHeight, -tileHalfDepth, -1.0f,  0.0f,  0.0f,  0.0f, 0.0f
    };
    
    glGenVertexArrays(1, &liftedTileVAO);
    glGenBuffers(1, &liftedTileVBO);
    glBindVertexArray(liftedTileVAO);
    glBindBuffer(GL_ARRAY_BUFFER, liftedTileVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(liftedTileVertices), liftedTileVertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);
    glBindVertexArray(0);

    // --- 创建无盖长方体（洞）的几何体 ---
    // 位于排除区域的正下方，向下延伸到地板下方
    // 尺寸：宽度1.5，深度2.0，高度0.65
    // 顶部Y: -2.85（地板顶部），底部Y: -3.5（地板下方，更深）
    // 无盖长方体：5个面（底部、前面、后面、左面、右面，没有顶部）
    float holeWidth = 1.5f;   // X方向宽度
    float holeDepth = 2.0f;   // Z方向深度
    float holeTopY = -2.85f;  // 顶部Y坐标（地板顶部）
    float holeBottomY = -3.5f; // 底部Y坐标（更深，延伸到地板下方）
    float holeHeight = holeTopY - holeBottomY; // 高度 = 0.65
    
    float holeCenterY = (holeTopY + holeBottomY) * 0.5f; // 洞的中心Y（X和Z在渲染时通过tileWorldCenterX/Z设置）
    // 保存到全局变量，供processInput使用
    g_holeCenterY = holeCenterY;
    
    // 局部坐标范围（中心在原点）
    float holeHalfWidth = holeWidth * 0.5f;  // 0.75
    float holeHalfDepth = holeDepth * 0.5f;  // 1.0
    float holeHalfHeight = holeHeight * 0.5f; // 0.075
    
    // 无盖长方体的顶点（5个面，30个顶点）
    float liftedTileHoleVertices[] = {
        // 底部面（向下，Y=holeBottomY）
        -holeHalfWidth, -holeHalfHeight, -holeHalfDepth,  0.0f, -1.0f, 0.0f,  0.0f, 0.0f,
         holeHalfWidth, -holeHalfHeight, -holeHalfDepth,  0.0f, -1.0f, 0.0f,  1.0f, 0.0f,
         holeHalfWidth, -holeHalfHeight,  holeHalfDepth,  0.0f, -1.0f, 0.0f,  1.0f, 1.0f,
         holeHalfWidth, -holeHalfHeight,  holeHalfDepth,  0.0f, -1.0f, 0.0f,  1.0f, 1.0f,
        -holeHalfWidth, -holeHalfHeight,  holeHalfDepth,  0.0f, -1.0f, 0.0f,  0.0f, 1.0f,
        -holeHalfWidth, -holeHalfHeight, -holeHalfDepth,  0.0f, -1.0f, 0.0f,  0.0f, 0.0f,
        
        // 前面（+Z方向，朝向房间前面）
        -holeHalfWidth, -holeHalfHeight, holeHalfDepth,  0.0f,  0.0f,  1.0f,  0.0f, 0.0f,
         holeHalfWidth, -holeHalfHeight, holeHalfDepth,  0.0f,  0.0f,  1.0f,  1.0f, 0.0f,
         holeHalfWidth,  holeHalfHeight, holeHalfDepth,  0.0f,  0.0f,  1.0f,  1.0f, 1.0f,
         holeHalfWidth,  holeHalfHeight, holeHalfDepth,  0.0f,  0.0f,  1.0f,  1.0f, 1.0f,
        -holeHalfWidth,  holeHalfHeight, holeHalfDepth,  0.0f,  0.0f,  1.0f,  0.0f, 1.0f,
        -holeHalfWidth, -holeHalfHeight, holeHalfDepth,  0.0f,  0.0f,  1.0f,  0.0f, 0.0f,
        
        // 后面（-Z方向，朝向房间后面）
        -holeHalfWidth, -holeHalfHeight, -holeHalfDepth,  0.0f,  0.0f, -1.0f,  0.0f, 0.0f,
         holeHalfWidth, -holeHalfHeight, -holeHalfDepth,  0.0f,  0.0f, -1.0f,  1.0f, 0.0f,
         holeHalfWidth,  holeHalfHeight, -holeHalfDepth,  0.0f,  0.0f, -1.0f,  1.0f, 1.0f,
         holeHalfWidth,  holeHalfHeight, -holeHalfDepth,  0.0f,  0.0f, -1.0f,  1.0f, 1.0f,
        -holeHalfWidth,  holeHalfHeight, -holeHalfDepth,  0.0f,  0.0f, -1.0f,  0.0f, 1.0f,
        -holeHalfWidth, -holeHalfHeight, -holeHalfDepth,  0.0f,  0.0f, -1.0f,  0.0f, 0.0f,
        
        // 右面（+X方向）
         holeHalfWidth, -holeHalfHeight, -holeHalfDepth,  1.0f,  0.0f,  0.0f,  0.0f, 0.0f,
         holeHalfWidth, -holeHalfHeight,  holeHalfDepth,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f,
         holeHalfWidth,  holeHalfHeight,  holeHalfDepth,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f,
         holeHalfWidth,  holeHalfHeight,  holeHalfDepth,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f,
         holeHalfWidth,  holeHalfHeight, -holeHalfDepth,  1.0f,  0.0f,  0.0f,  0.0f, 1.0f,
         holeHalfWidth, -holeHalfHeight, -holeHalfDepth,  1.0f,  0.0f,  0.0f,  0.0f, 0.0f,
        
        // 左面（-X方向）
        -holeHalfWidth, -holeHalfHeight, -holeHalfDepth, -1.0f,  0.0f,  0.0f,  0.0f, 0.0f,
        -holeHalfWidth, -holeHalfHeight,  holeHalfDepth, -1.0f,  0.0f,  0.0f,  1.0f, 0.0f,
        -holeHalfWidth,  holeHalfHeight,  holeHalfDepth, -1.0f,  0.0f,  0.0f,  1.0f, 1.0f,
        -holeHalfWidth,  holeHalfHeight,  holeHalfDepth, -1.0f,  0.0f,  0.0f,  1.0f, 1.0f,
        -holeHalfWidth,  holeHalfHeight, -holeHalfDepth, -1.0f,  0.0f,  0.0f,  0.0f, 1.0f,
        -holeHalfWidth, -holeHalfHeight, -holeHalfDepth, -1.0f,  0.0f,  0.0f,  0.0f, 0.0f
    };
    
    glGenVertexArrays(1, &liftedTileHoleVAO);
    glGenBuffers(1, &liftedTileHoleVBO);
    glBindVertexArray(liftedTileHoleVAO);
    glBindBuffer(GL_ARRAY_BUFFER, liftedTileHoleVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(liftedTileHoleVertices), liftedTileHoleVertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);
    glBindVertexArray(0);

    // --- 创建书柜后面墙上的砖墙正方形（用于右墙上的小方块） ---
    // 正方形大小：1.0 x 1.0
    // 位置：书柜后面，右墙内表面（x=3.95），高度在书柜中心附近
    float brickSquareSize = 1.0f; // 正方形边长
    float brickSquareHalfSize = brickSquareSize * 0.5f;
    // 书柜位置：x=3.95, y=-2.90, z=2.0
    // 砖墙方块位置：在书柜后面，z稍微靠后一些，比如z=2.5，高度与书柜中心对齐
    float brickSquareVertices[] = {
        // 位置(3) + 法线(3) + 纹理坐标(2) = 8个float
        // 正方形在YZ平面（X=0），法线指向-X方向（朝向房间内部）
        // 顶点顺序：左下、右下、右上、右上、左上、左下（逆时针，从房间内部看）
        0.0f, -brickSquareHalfSize, -brickSquareHalfSize,  -1.0f, 0.0f, 0.0f,  0.0f, 0.0f,  // 左下
        0.0f, -brickSquareHalfSize,  brickSquareHalfSize,  -1.0f, 0.0f, 0.0f,  1.0f, 0.0f,  // 右下
        0.0f,  brickSquareHalfSize,  brickSquareHalfSize,  -1.0f, 0.0f, 0.0f,  1.0f, 1.0f,  // 右上
        0.0f,  brickSquareHalfSize,  brickSquareHalfSize,  -1.0f, 0.0f, 0.0f,  1.0f, 1.0f,  // 右上
        0.0f,  brickSquareHalfSize, -brickSquareHalfSize,  -1.0f, 0.0f, 0.0f,  0.0f, 1.0f,  // 左上
        0.0f, -brickSquareHalfSize, -brickSquareHalfSize,  -1.0f, 0.0f, 0.0f,  0.0f, 0.0f   // 左下
    };
    
    unsigned int brickSquareVAO, brickSquareVBO;
    glGenVertexArrays(1, &brickSquareVAO);
    glGenBuffers(1, &brickSquareVBO);
    glBindVertexArray(brickSquareVAO);
    glBindBuffer(GL_ARRAY_BUFFER, brickSquareVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(brickSquareVertices), brickSquareVertices, GL_STATIC_DRAW);
    // 位置属性
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    // 法线属性
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    // 纹理坐标属性
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);
    glBindVertexArray(0);

    // --- 创建书柜按钮的正方体几何体 ---
    // 按钮大小：0.2 x 0.2 x 0.2
    float buttonSize = 0.2f;
    float buttonHalfSize = buttonSize * 0.5f;
    // 按钮在右墙上，在YZ平面（X=0），法线指向-X方向
    float buttonVertices[] = {
        // 位置(3) + 法线(3) + 纹理坐标(2) = 8个float
        // 前面（朝向房间内部，法线指向-X方向）
        0.0f, -buttonHalfSize, -buttonHalfSize,  -1.0f, 0.0f, 0.0f,  0.0f, 0.0f,  // 左下
        0.0f, -buttonHalfSize,  buttonHalfSize,  -1.0f, 0.0f, 0.0f,  1.0f, 0.0f,  // 右下
        0.0f,  buttonHalfSize,  buttonHalfSize,  -1.0f, 0.0f, 0.0f,  1.0f, 1.0f,  // 右上
        0.0f,  buttonHalfSize,  buttonHalfSize,  -1.0f, 0.0f, 0.0f,  1.0f, 1.0f,  // 右上
        0.0f,  buttonHalfSize, -buttonHalfSize,  -1.0f, 0.0f, 0.0f,  0.0f, 1.0f,  // 左上
        0.0f, -buttonHalfSize, -buttonHalfSize,  -1.0f, 0.0f, 0.0f,  0.0f, 0.0f,  // 左下
        // 后面（背向房间内部，法线指向+X方向）
        0.0f, -buttonHalfSize, -buttonHalfSize,  1.0f, 0.0f, 0.0f,  0.0f, 0.0f,
        0.0f,  buttonHalfSize, -buttonHalfSize,  1.0f, 0.0f, 0.0f,  1.0f, 0.0f,
        0.0f,  buttonHalfSize,  buttonHalfSize,  1.0f, 0.0f, 0.0f,  1.0f, 1.0f,
        0.0f,  buttonHalfSize,  buttonHalfSize,  1.0f, 0.0f, 0.0f,  1.0f, 1.0f,
        0.0f, -buttonHalfSize,  buttonHalfSize,  1.0f, 0.0f, 0.0f,  0.0f, 1.0f,
        0.0f, -buttonHalfSize, -buttonHalfSize,  1.0f, 0.0f, 0.0f,  0.0f, 0.0f,
        // 顶部（法线指向+Y方向）
        0.0f,  buttonHalfSize, -buttonHalfSize,  0.0f, 1.0f, 0.0f,  0.0f, 0.0f,
        0.0f,  buttonHalfSize,  buttonHalfSize,  0.0f, 1.0f, 0.0f,  1.0f, 0.0f,
        0.0f,  buttonHalfSize,  buttonHalfSize,  0.0f, 1.0f, 0.0f,  1.0f, 1.0f,
        0.0f,  buttonHalfSize,  buttonHalfSize,  0.0f, 1.0f, 0.0f,  1.0f, 1.0f,
        0.0f,  buttonHalfSize, -buttonHalfSize,  0.0f, 1.0f, 0.0f,  0.0f, 1.0f,
        0.0f,  buttonHalfSize, -buttonHalfSize,  0.0f, 1.0f, 0.0f,  0.0f, 0.0f,
        // 底部（法线指向-Y方向）
        0.0f, -buttonHalfSize, -buttonHalfSize,  0.0f, -1.0f, 0.0f,  0.0f, 0.0f,
        0.0f, -buttonHalfSize,  buttonHalfSize,  0.0f, -1.0f, 0.0f,  1.0f, 0.0f,
        0.0f, -buttonHalfSize,  buttonHalfSize,  0.0f, -1.0f, 0.0f,  1.0f, 1.0f,
        0.0f, -buttonHalfSize,  buttonHalfSize,  0.0f, -1.0f, 0.0f,  1.0f, 1.0f,
        0.0f, -buttonHalfSize, -buttonHalfSize,  0.0f, -1.0f, 0.0f,  0.0f, 1.0f,
        0.0f, -buttonHalfSize, -buttonHalfSize,  0.0f, -1.0f, 0.0f,  0.0f, 0.0f,
        // 右面（法线指向+Z方向）
        0.0f, -buttonHalfSize,  buttonHalfSize,  0.0f, 0.0f, 1.0f,  0.0f, 0.0f,
        0.0f,  buttonHalfSize,  buttonHalfSize,  0.0f, 0.0f, 1.0f,  1.0f, 0.0f,
        0.0f,  buttonHalfSize,  buttonHalfSize,  0.0f, 0.0f, 1.0f,  1.0f, 1.0f,
        0.0f,  buttonHalfSize,  buttonHalfSize,  0.0f, 0.0f, 1.0f,  1.0f, 1.0f,
        0.0f, -buttonHalfSize,  buttonHalfSize,  0.0f, 0.0f, 1.0f,  0.0f, 1.0f,
        0.0f, -buttonHalfSize,  buttonHalfSize,  0.0f, 0.0f, 1.0f,  0.0f, 0.0f,
        // 左面（法线指向-Z方向）
        0.0f, -buttonHalfSize, -buttonHalfSize,  0.0f, 0.0f, -1.0f,  0.0f, 0.0f,
        0.0f,  buttonHalfSize, -buttonHalfSize,  0.0f, 0.0f, -1.0f,  1.0f, 0.0f,
        0.0f,  buttonHalfSize, -buttonHalfSize,  0.0f, 0.0f, -1.0f,  1.0f, 1.0f,
        0.0f,  buttonHalfSize, -buttonHalfSize,  0.0f, 0.0f, -1.0f,  1.0f, 1.0f,
        0.0f, -buttonHalfSize, -buttonHalfSize,  0.0f, 0.0f, -1.0f,  0.0f, 1.0f,
        0.0f, -buttonHalfSize, -buttonHalfSize,  0.0f, 0.0f, -1.0f,  0.0f, 0.0f
    };
    
    unsigned int buttonVAO, buttonVBO;
    glGenVertexArrays(1, &buttonVAO);
    glGenBuffers(1, &buttonVBO);
    glBindVertexArray(buttonVAO);
    glBindBuffer(GL_ARRAY_BUFFER, buttonVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(buttonVertices), buttonVertices, GL_STATIC_DRAW);
    // 位置属性
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    // 法线属性
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    // 纹理坐标属性
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);
    glBindVertexArray(0);

    // --- 生成球体几何体（用于灵珠） ---
    std::vector<float> sphereVertices;
    std::vector<unsigned int> sphereIndices;
    unsigned int sphereVAO, sphereVBO, sphereEBO;
    unsigned int sphereIndexCount = 0;
    
    // 球体参数
    float sphereRadius = 0.3f;
    int sphereSectors = 32;  // 经度分段数
    int sphereStacks = 32;   // 纬度分段数
    
    // 生成球体顶点
    for (int i = 0; i <= sphereStacks; ++i) {
        float stackAngle = M_PI / 2.0f - i * M_PI / sphereStacks;
        float xy = sphereRadius * cosf(stackAngle);
        float z = sphereRadius * sinf(stackAngle);
        
        for (int j = 0; j <= sphereSectors; ++j) {
            float sectorAngle = j * 2.0f * M_PI / sphereSectors;
            
            float x = xy * cosf(sectorAngle);
            float y = xy * sinf(sectorAngle);
            
            // 位置
            sphereVertices.push_back(x);
            sphereVertices.push_back(y);
            sphereVertices.push_back(z);
            
            // 法线（归一化）
            float nx = x / sphereRadius;
            float ny = y / sphereRadius;
            float nz = z / sphereRadius;
            sphereVertices.push_back(nx);
            sphereVertices.push_back(ny);
            sphereVertices.push_back(nz);
        }
    }
    
    // 生成球体索引
    for (int i = 0; i < sphereStacks; ++i) {
        int k1 = i * (sphereSectors + 1);
        int k2 = k1 + sphereSectors + 1;
        
        for (int j = 0; j < sphereSectors; ++j, ++k1, ++k2) {
            if (i != 0) {
                sphereIndices.push_back(k1);
                sphereIndices.push_back(k2);
                sphereIndices.push_back(k1 + 1);
            }
            
            if (i != (sphereStacks - 1)) {
                sphereIndices.push_back(k1 + 1);
                sphereIndices.push_back(k2);
                sphereIndices.push_back(k2 + 1);
            }
        }
    }
    sphereIndexCount = sphereIndices.size();
    
    // 创建球体VAO
    glGenVertexArrays(1, &sphereVAO);
    glGenBuffers(1, &sphereVBO);
    glGenBuffers(1, &sphereEBO);
    
    glBindVertexArray(sphereVAO);
    glBindBuffer(GL_ARRAY_BUFFER, sphereVBO);
    glBufferData(GL_ARRAY_BUFFER, sphereVertices.size() * sizeof(float), &sphereVertices[0], GL_STATIC_DRAW);
    
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, sphereEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sphereIndices.size() * sizeof(unsigned int), &sphereIndices[0], GL_STATIC_DRAW);
    
    // 位置属性
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    // 法线属性
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    
    glBindVertexArray(0);

    // --- 创建全屏四边形（用于后处理） ---
    float quadVertices[] = {
        // positions   // texCoords
        -1.0f,  1.0f,  0.0f, 1.0f,
        -1.0f, -1.0f,  0.0f, 0.0f,
        1.0f, -1.0f,  1.0f, 0.0f,
        -1.0f,  1.0f,  0.0f, 1.0f,
        1.0f, -1.0f,  1.0f, 0.0f,
        1.0f,  1.0f,  1.0f, 1.0f
    };

    glGenVertexArrays(1, &quadVAO);
    glGenBuffers(1, &quadVBO);
    glBindVertexArray(quadVAO);
    glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glBindVertexArray(0);

    // --- 创建"E"字的几何体（世界空间，使用billboard） ---
    // 使用一个简单的quad，billboard着色器会处理E字的绘制
    float eQuadVertices[] = {
        // 位置（相对于中心的偏移，-0.5到0.5）
        -0.5f, -0.5f, 0.0f,  // 左下
         0.5f, -0.5f, 0.0f,  // 右下
         0.5f,  0.5f, 0.0f,  // 右上
        -0.5f,  0.5f, 0.0f   // 左上
    };
    
    unsigned int eIndices[] = {
        0, 1, 2,
        2, 3, 0
    };
    
    unsigned int eVAO, eVBO, eEBO;
    glGenVertexArrays(1, &eVAO);
    glGenBuffers(1, &eVBO);
    glGenBuffers(1, &eEBO);
    glBindVertexArray(eVAO);
    glBindBuffer(GL_ARRAY_BUFFER, eVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(eQuadVertices), eQuadVertices, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, eEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(eIndices), eIndices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glBindVertexArray(0);

    // --- 创建屏幕空间消息显示的quad（用于显示"已拾取灵珠！"） ---
    // 屏幕空间坐标（像素坐标）+ 纹理坐标
    float messageQuadWidth = 600.0f;  // 消息框宽度（放大）
    float messageQuadHeight = 300.0f; // 消息框高度（放大）
    float centerX = SCR_WIDTH / 2.0f;
    float centerY = SCR_HEIGHT / 2.0f;
    
    // 顶点数据：位置(2) + 纹理坐标(2) = 4个float
    float messageQuadVertices[] = {
        // 屏幕空间坐标（像素坐标）    // 纹理坐标
        centerX - messageQuadWidth / 2.0f, centerY - messageQuadHeight / 2.0f,  0.0f, 0.0f,  // 左下
        centerX + messageQuadWidth / 2.0f, centerY - messageQuadHeight / 2.0f,  1.0f, 0.0f,  // 右下
        centerX + messageQuadWidth / 2.0f, centerY + messageQuadHeight / 2.0f,  1.0f, 1.0f,  // 右上
        centerX - messageQuadWidth / 2.0f, centerY + messageQuadHeight / 2.0f,  0.0f, 1.0f   // 左上
    };
    
    unsigned int messageQuadIndices[] = {
        0, 1, 2,
        2, 3, 0
    };
    
    unsigned int messageQuadVAO, messageQuadVBO, messageQuadEBO;
    glGenVertexArrays(1, &messageQuadVAO);
    glGenBuffers(1, &messageQuadVBO);
    glGenBuffers(1, &messageQuadEBO);
    glBindVertexArray(messageQuadVAO);
    glBindBuffer(GL_ARRAY_BUFFER, messageQuadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(messageQuadVertices), messageQuadVertices, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, messageQuadEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(messageQuadIndices), messageQuadIndices, GL_STATIC_DRAW);
    // 位置属性
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    // 纹理坐标属性
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glBindVertexArray(0);

    // 统一房间用的顶点信息(每一个前面三个值为顶点坐标，中间三个值为法线向量，最后两个值为纹理坐标)
    // ------------------------------------------------------------------
    float vertices[] = {
        // positions          // normals           // texCoords
        -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f, 0.0f,
         0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f, 0.0f,
         0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f, 1.0f,
         0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f, 1.0f,
        -0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f, 1.0f,
        -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f, 0.0f,

        -0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f, 0.0f,
         0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f, 0.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f, 1.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f, 1.0f,
        -0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f, 1.0f,
        -0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f, 0.0f,

        -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  1.0f, 0.0f,
        -0.5f,  0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  1.0f, 1.0f,
        -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  0.0f, 1.0f,
        -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  0.0f, 1.0f,
        -0.5f, -0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  0.0f, 0.0f,
        -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  1.0f, 0.0f,

         0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f,
         0.5f,  0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f,
         0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  0.0f, 1.0f,
         0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  0.0f, 1.0f,
         0.5f, -0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  0.0f, 0.0f,
         0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f,

        -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  0.0f, 1.0f,
         0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  1.0f, 1.0f,
         0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  1.0f, 0.0f,
         0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  1.0f, 0.0f,
        -0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  0.0f, 0.0f,
        -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  0.0f, 1.0f,

        -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  0.0f, 1.0f,
         0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  1.0f, 1.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  1.0f, 0.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  1.0f, 0.0f,
        -0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  0.0f, 0.0f,
        -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  0.0f, 1.0f
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
    int wallWithHoleVertexCount = wallWithHoleVertices.size() / 8;

    // 创建墙壁、地板、天花板的VAO/VBO
    // ------------------------------------------------------------------
    unsigned int roomVAO, roomVBO;
    glGenVertexArrays(1, &roomVAO);
    glGenBuffers(1, &roomVBO);
    glBindVertexArray(roomVAO);
    glBindBuffer(GL_ARRAY_BUFFER, roomVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    // 位置属性
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    // 法线属性
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    // 纹理坐标属性
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);

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
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    
    // 创建后墙的VAO/VBO
    // ------------------------------------------------------------------
    unsigned int wallVAO, wallVBO;
    glGenVertexArrays(1, &wallVAO);
    glGenBuffers(1, &wallVBO);
    glBindVertexArray(wallVAO);
    glBindBuffer(GL_ARRAY_BUFFER, wallVBO);
    glBufferData(GL_ARRAY_BUFFER, wallWithHoleVertices.size() * sizeof(float), wallWithHoleVertices.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);

    // --- 初始化 Bloom FBO ---
    // 创建 HDR FBO（用于渲染场景）
    glGenFramebuffers(1, &hdrFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, hdrFBO);

    // 创建两个颜色缓冲（多渲染目标）
    glGenTextures(2, colorBuffers);
    for (unsigned int i = 0; i < 2; i++)
    {
        glBindTexture(GL_TEXTURE_2D, colorBuffers[i]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGB, GL_FLOAT, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, GL_TEXTURE_2D, colorBuffers[i], 0);
    }

    // 创建深度缓冲
    unsigned int rboDepth;
    glGenRenderbuffers(1, &rboDepth);
    glBindRenderbuffer(GL_RENDERBUFFER, rboDepth);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, SCR_WIDTH, SCR_HEIGHT);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rboDepth);

    // 告诉 OpenGL 使用多个颜色附件
    unsigned int attachments[2] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
    glDrawBuffers(2, attachments);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        std::cout << "ERROR::FRAMEBUFFER:: HDR FBO is not complete!" << std::endl;
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // 创建 Ping-Pong FBO（用于模糊）
    glGenFramebuffers(2, pingpongFBO);
    glGenTextures(2, pingpongColorBuffers);
    for (unsigned int i = 0; i < 2; i++)
    {
        glBindFramebuffer(GL_FRAMEBUFFER, pingpongFBO[i]);
        glBindTexture(GL_TEXTURE_2D, pingpongColorBuffers[i]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGB, GL_FLOAT, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, pingpongColorBuffers[i], 0);
        
        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
            std::cout << "ERROR::FRAMEBUFFER:: PingPong FBO is not complete!" << std::endl;
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

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
        
        // 更新拾取消息显示时间
        if (showOrbPickupMessage) {
            orbPickupMessageTime += deltaTime;
            if (orbPickupMessageTime >= ORB_PICKUP_MESSAGE_DURATION) {
                showOrbPickupMessage = false;
            }
        }

        // 雷电逻辑
        if (lightning) {
            lightning->Update(deltaTime, isRaining, sandbox_heightmap);
        }

        // 检测两个马是否都在对应的地砖范围内，触发地板翘起
        if (isHorse1OnTile1() && isHorse2OnTile2()) {
            if (!isFloorTileLifting) {
                isFloorTileLifting = true;
                std::cout << "触发地板翘起效果！" << std::endl;
            }
            // 更新翘起进度（0.0 到 1.0）- 正向旋转
            floorLiftProgress = glm::min(1.0f, floorLiftProgress + floorLiftSpeed * deltaTime);
        } else {
            // 如果马离开了触发区域，反向旋转回去
            if (isFloorTileLifting && floorLiftProgress > 0.0f) {
                // 减少翘起进度（反向旋转）
                floorLiftProgress = glm::max(0.0f, floorLiftProgress - floorLiftSpeed * deltaTime);
                // 当进度降到0时，重置状态
                if (floorLiftProgress <= 0.0f) {
                    isFloorTileLifting = false;
                    floorLiftProgress = 0.0f;
                    std::cout << "地板恢复原状" << std::endl;
                }
            }
        }

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
        float overallIntensity = 0.2f; // 整体亮度
        glm::vec3 finalLightColor = warmColor * overallIntensity;

        // --- 5. 如果打雷，增强光照 ---
        if (lightning && lightning->IsActive()) {
            finalLightColor = glm::vec3(0.8f, 0.9f, 1.0f) * 5.0f; // 蓝白色强光
        }

        // 确保在设置 Uniforms/Drawing 之前激活 Shader
        //---------------------------------------------------------------------
        lightingShader.use();
        lightingShader.setVec3("lightPos", lightPos); 
        lightingShader.setVec3("viewPos", camera.Position);
        lightingShader.setVec3("lightColor", finalLightColor);

        // 设置台灯点光源
        glm::vec3 lampLightPos = lampModelWorldPos + glm::vec3(0.0f, 0.4f, 0.0f);
        glm::vec3 lampColor(1.0f, 0.9f, 0.7f);
        lightingShader.setBool("lampOn", lampOn);
        lightingShader.setVec3("lampLight.position", lampLightPos);
        lightingShader.setVec3("lampLight.color",    lampColor);
        lightingShader.setFloat("lampLight.intensity", lampOn ? 4.0f : 0.0f);

        // 设置灵珠点光源（用于照亮无盖长方体内部）
        // 计算灵珠位置（与渲染灵珠时使用的位置相同）
        glm::vec3 orbLightPos = glm::vec3(g_tileWorldCenterX, g_holeCenterY, g_tileWorldCenterZ);
        // 计算灵珠颜色（与orb.fs中的计算方式相同，基于时间）
        glm::vec3 orbColor;
        orbColor.r = 0.5f + 0.5f * sinf(currentFrame * 2.0f + 0.0f);
        orbColor.g = 0.5f + 0.5f * sinf(currentFrame * 2.0f + 2.094f);
        orbColor.b = 0.5f + 0.5f * sinf(currentFrame * 2.0f + 4.189f);
        // 归一化颜色，使其更鲜艳（与orb.fs中的计算方式相同）
        if (glm::length(orbColor) > 0.001f) {
            glm::vec3 normalizedOrbColor = glm::normalize(orbColor);
            orbColor = normalizedOrbColor * 0.8f + orbColor * 0.2f;
        }
        // 只在触发翘起效果且未拾取时启用灵珠光源
        bool orbLightEnabled = (isFloorTileLifting && floorLiftProgress > 0.0f && !orbPicked);
        lightingShader.setBool("orbLightOn", orbLightEnabled);
        lightingShader.setVec3("orbLight.position", orbLightPos);
        lightingShader.setVec3("orbLight.color", orbColor);
        lightingShader.setFloat("orbLight.intensity", orbLightEnabled ? 8.0f : 0.0f); // 增加亮度

        glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
        glm::mat4 view = camera.GetViewMatrix();
        lightingShader.setMat4("projection", projection);
        lightingShader.setMat4("view", view);

        glm::mat4 model = glm::mat4(1.0f);

        glBindVertexArray(roomVAO);

        // 渲染到 HDR FBO
        glBindFramebuffer(GL_FRAMEBUFFER, hdrFBO);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // 绘制天花板
        {
            lightingShader.use();
            lightingShader.setVec3("objectColor", 1.0f, 1.0f, 1.0f); // 使用纹理时设为白色
            lightingShader.setBool("useTexture", true);
            lightingShader.setBool("isFloor", false); // 不是地板
            lightingShader.setBool("isHole", false); // 不是无盖长方体
            lightingShader.setBool("excludeBrickSquareRegion", false); // 不是右墙，不需要排除砖墙区域

            // 绑定木纹纹理
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, woodTexture);
            lightingShader.setInt("floorTexture", 0);

            // 设置模型变换
            model = glm::mat4(1.0f);
            model = glm::translate(model, glm::vec3(0.0f, 3.0f, 0.0f));
            model = glm::scale(model, glm::vec3(8.0f, 0.1f, 8.0f));
            lightingShader.setMat4("model", model);

            // 渲染
            glBindVertexArray(roomVAO);
            glDrawArrays(GL_TRIANGLES, 0, 36);
        }

        // 绘制地板
        {
            lightingShader.use();
            lightingShader.setVec3("objectColor", 1.0f, 1.0f, 1.0f); // 使用纹理时设为白色
            lightingShader.setBool("useTexture", true);
            lightingShader.setBool("isFloor", true); // 标识这是地板
            lightingShader.setBool("isLiftedTile", false); // 这是主地板，不是翘起地板块
            lightingShader.setBool("isHole", false); // 不是无盖长方体
            lightingShader.setBool("excludeBrickSquareRegion", false); // 不是右墙，不需要排除砖墙区域

            // 设置翘起区域排除参数（如果翘起地板块正在渲染）
            if (isFloorTileLifting && floorLiftProgress > 0.0f) {
                lightingShader.setBool("excludeLiftedTileRegion", true);
                // 翘起地板块的世界坐标范围（向右平移0.5个单位，向z轴负方向移动1.0个单位后）
                // 排除区域：X: 2.0 到 3.5, Z: -2.5 到 -0.5
                // 排除区域的中心：(2.75, -1.5) - 已在初始化时设置到全局变量g_tileWorldCenterX和g_tileWorldCenterZ
                lightingShader.setVec2("liftedTileRegionMin", glm::vec2(2.0f, -2.5f));
                lightingShader.setVec2("liftedTileRegionMax", glm::vec2(3.5f, -0.5f));
            } else {
                lightingShader.setBool("excludeLiftedTileRegion", false);
            }
            
            // 排除地砖区域（z轴尽头的两块地砖）
            lightingShader.setBool("excludeBrickTileRegion", true);
            // 地砖1区域：X: -3.0 到 -1.0, Z: 2.0 到 4.0
            lightingShader.setVec2("brickTile1RegionMin", glm::vec2(BRICK_TILE1_MIN_X, BRICK_TILE1_MIN_Z));
            lightingShader.setVec2("brickTile1RegionMax", glm::vec2(BRICK_TILE1_MAX_X, BRICK_TILE1_MAX_Z));
            // 地砖2区域：X: 1.0 到 3.0, Z: 2.0 到 4.0
            lightingShader.setVec2("brickTile2RegionMin", glm::vec2(BRICK_TILE2_MIN_X, BRICK_TILE2_MIN_Z));
            lightingShader.setVec2("brickTile2RegionMax", glm::vec2(BRICK_TILE2_MAX_X, BRICK_TILE2_MAX_Z));

            // 绑定地砖纹理
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, floorTexture);
            lightingShader.setInt("floorTexture", 0);

            // 设置模型变换
            model = glm::mat4(1.0f);
            model = glm::translate(model, glm::vec3(0.0f, -2.50f, 0.0f));
            model = glm::scale(model, glm::vec3(8.0f, 1.0f, 8.0f));
            lightingShader.setMat4("model", model);

            // 使用地板的VAO渲染（现在有36个顶点，6个面）
            glBindVertexArray(floorVAO);
            glDrawArrays(GL_TRIANGLES, 0, 36);
            glBindVertexArray(0);
        }

        // 绘制翘起的地板块（如果触发）
        if (isFloorTileLifting && floorLiftProgress > 0.0f) {
            lightingShader.use();
            lightingShader.setVec3("objectColor", 1.0f, 1.0f, 1.0f);
            lightingShader.setBool("useTexture", true);
            lightingShader.setBool("isFloor", true);
            lightingShader.setBool("isLiftedTile", true); // 这是翘起地板块，不应该被排除
            lightingShader.setBool("isHole", false); // 不是无盖长方体
            lightingShader.setBool("excludeBrickSquareRegion", false); // 不是右墙，不需要排除砖墙区域
            lightingShader.setBool("excludeLiftedTileRegion", false); // 翘起地板块本身不需要排除
            lightingShader.setBool("excludeBrickTileRegion", false); // 翘起地板块不需要排除地砖区域

            // 绑定地砖纹理
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, floorTexture);
            lightingShader.setInt("floorTexture", 0);

            // 计算翘起角度（最大90度）
            float maxLiftAngle = 90.0f;
            float liftAngle = floorLiftProgress * maxLiftAngle;

            // 设置模型变换：变换顺序（从右到左执行，代码从左到右写）：
            // 1. 平移到世界位置（地板块中心）
            // 2. 平移到旋转轴位置（前边缘的地板表面，z=tileHalfDepth, y=tileHalfHeight，作为支点）
            // 3. 绕x轴旋转（正角度，让后边缘向上翘起）
            // 4. 平移回中心（相对于前边缘地板表面的偏移）
            model = glm::mat4(1.0f);
            // 先平移到世界位置（地板块中心，使用与无盖长方体相同的全局变量位置）
            model = glm::translate(model, glm::vec3(g_tileWorldCenterX, tileWorldCenterY, g_tileWorldCenterZ));
            // 平移到旋转轴位置（前边缘的地板表面）
            // 局部坐标中，前边缘在z=tileHalfDepth=1.0，地板表面在y=tileHalfHeight=0.075
            // 支点应该在前边缘且在地板表面（不是地板块的中心y=0）
            model = glm::translate(model, glm::vec3(0.0f, tileHalfHeight, tileHalfDepth));
            // 绕x轴旋转（使用正角度，让后边缘向上翘起）
            // 旋转角度为正，使得后边缘（z=-tileHalfDepth）向上旋转
            model = glm::rotate(model, glm::radians(liftAngle), glm::vec3(1.0f, 0.0f, 0.0f));
            // 平移回中心（相对于前边缘地板表面的偏移）
            model = glm::translate(model, glm::vec3(0.0f, -tileHalfHeight, -tileHalfDepth));
            lightingShader.setMat4("model", model);

            // 渲染翘起的地板块
            glBindVertexArray(liftedTileVAO);
            glDrawArrays(GL_TRIANGLES, 0, 36); // 翘起的地板块也有36个顶点（6个面）
            glBindVertexArray(0);
        }

        // 绘制无盖长方体（洞，如果触发）
        if (isFloorTileLifting && floorLiftProgress > 0.0f) {
            lightingShader.use();
            lightingShader.setVec3("objectColor", 1.0f, 1.0f, 1.0f);
            lightingShader.setBool("useTexture", true);
            lightingShader.setBool("isFloor", true); // 使用地板纹理
            lightingShader.setBool("isLiftedTile", false);
            lightingShader.setBool("isHole", true); // 标识这是无盖长方体（洞）
            lightingShader.setBool("excludeBrickSquareRegion", false); // 不是右墙，不需要排除砖墙区域
            lightingShader.setBool("excludeLiftedTileRegion", false);
            lightingShader.setBool("excludeBrickTileRegion", false); // 无盖长方体不需要排除地砖区域

            // 绑定地板纹理
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, floorTexture);
            lightingShader.setInt("floorTexture", 0);

            // 设置模型变换（平移到翘起地板块的正下方，与翘起地板块位置对齐）
            model = glm::mat4(1.0f);
            model = glm::translate(model, glm::vec3(g_tileWorldCenterX, g_holeCenterY, g_tileWorldCenterZ));
            lightingShader.setMat4("model", model);

            // 渲染无盖长方体（5个面，30个顶点）
            glBindVertexArray(liftedTileHoleVAO);
            glDrawArrays(GL_TRIANGLES, 0, 30);
            glBindVertexArray(0);
        }

        // --- 绘制z轴尽头的地砖（两块） ---
        {
            lightingShader.use();
            lightingShader.setVec3("objectColor", 1.0f, 1.0f, 1.0f);
            lightingShader.setBool("useTexture", true);
            lightingShader.setBool("isFloor", true);
            lightingShader.setBool("isLiftedTile", false);
            lightingShader.setBool("isHole", false);
            lightingShader.setBool("excludeBrickSquareRegion", false);
            lightingShader.setBool("excludeLiftedTileRegion", false);
            lightingShader.setBool("excludeBrickTileRegion", false); // 地砖本身不需要排除
            
            // 绑定砖墙纹理
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, brickWallTexture);
            lightingShader.setInt("floorTexture", 0);
            
            // 绘制地砖1（左侧，对应马1）
            model = glm::mat4(1.0f);
            model = glm::translate(model, glm::vec3(BRICK_TILE1_CENTER_X, BRICK_TILE_Y, BRICK_TILE_Z));
            lightingShader.setMat4("model", model);
            glBindVertexArray(brickTile1VAO);
            glDrawArrays(GL_TRIANGLES, 0, 36);
            
            // 绘制地砖2（右侧，对应马2）
            model = glm::mat4(1.0f);
            model = glm::translate(model, glm::vec3(BRICK_TILE2_CENTER_X, BRICK_TILE_Y, BRICK_TILE_Z));
            lightingShader.setMat4("model", model);
            glBindVertexArray(brickTile2VAO);
            glDrawArrays(GL_TRIANGLES, 0, 36);
            
            glBindVertexArray(0);
        }

        // 绘制左墙
        {
            lightingShader.use();
            lightingShader.setVec3("objectColor", 1.0f, 1.0f, 1.0f); // 使用纹理时设为白色
            lightingShader.setBool("useTexture", true);
            lightingShader.setBool("isFloor", false); // 不是地板
            lightingShader.setBool("isHole", false); // 不是无盖长方体
            lightingShader.setBool("excludeBrickSquareRegion", false); // 不是右墙，不需要排除砖墙区域

            // 绑定木纹纹理
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, woodTexture);
            lightingShader.setInt("floorTexture", 0);

            // 重新绑定roomVAO（因为地板使用了floorVAO）
            glBindVertexArray(roomVAO);

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
            lightingShader.use();
            lightingShader.setVec3("objectColor", 1.0f, 1.0f, 1.0f); // 使用纹理时设为白色
            lightingShader.setBool("useTexture", true);
            lightingShader.setBool("isFloor", false); // 不是地板
            lightingShader.setBool("isHole", false); // 不是无盖长方体
            lightingShader.setBool("isBrickSquare", false); // 不是砖墙正方形

            // 设置砖墙区域排除参数
            // 砖墙正方形位置：x=3.94, y=-2.20, z=2.2，大小1.0x1.0
            // 所以Y范围：-2.20 ± 0.5 = -2.70 到 -1.70
            // Z范围：2.2 ± 0.5 = 1.7 到 2.7
            lightingShader.setBool("excludeBrickSquareRegion", true);
            lightingShader.setVec2("brickSquareRegionMin", glm::vec2(-2.70f, 1.7f));  // (y, z) 最小边界
            lightingShader.setVec2("brickSquareRegionMax", glm::vec2(-1.70f, 2.7f));  // (y, z) 最大边界

            // 绑定木纹纹理
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, woodTexture);
            lightingShader.setInt("floorTexture", 0);

            // 设置模型变换
            model = glm::mat4(1.0f);
            model = glm::translate(model, glm::vec3(4.0f, 0.0f, 0.0f));
            model = glm::scale(model, glm::vec3(0.1f, 6.0f, 8.0f));
            lightingShader.setMat4("model", model);

            // 渲染（roomVAO已经在左墙时绑定了）
            glDrawArrays(GL_TRIANGLES, 0, 36);
        }

        // --- 绘制书柜后面的砖墙正方形（在右墙上） ---
        {
            lightingShader.use();
            // 确保设置必要的uniform（projection和view在循环开始时已设置，但需要确保它们仍然有效）
            lightingShader.setMat4("projection", projection);
            lightingShader.setMat4("view", view);
            lightingShader.setVec3("lightPos", lightPos);
            lightingShader.setVec3("viewPos", camera.Position);
            lightingShader.setVec3("lightColor", finalLightColor);
            // 设置台灯点光源
            glm::vec3 lampLightPos = lampModelWorldPos + glm::vec3(0.0f, 0.4f, 0.0f);
            glm::vec3 lampColor(1.0f, 0.9f, 0.7f);
            lightingShader.setBool("lampOn", lampOn);
            lightingShader.setVec3("lampLight.position", lampLightPos);
            lightingShader.setVec3("lampLight.color", lampColor);
            lightingShader.setFloat("lampLight.intensity", lampOn ? 4.0f : 0.0f);
            
            lightingShader.setVec3("objectColor", 1.0f, 1.0f, 1.0f);
            lightingShader.setBool("useTexture", true);
            lightingShader.setBool("isFloor", false);
            lightingShader.setBool("isHole", false);
            lightingShader.setBool("isBrickSquare", true); // 标识这是砖墙正方形，不应该被排除
            lightingShader.setBool("excludeBrickSquareRegion", false); // 砖墙正方形本身不需要排除

            // 绑定砖墙纹理
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, brickTexture);
            lightingShader.setInt("floorTexture", 0);

            // 设置模型变换
            // 书柜位置：x=3.45, y=-2.90, z=2.0
            // 右墙内表面在 x=3.95（右墙中心x=4.0，厚度0.1，所以内表面在4.0-0.05=3.95）
            // 砖墙方块：稍微突出墙面（x=3.94），避免与右墙内表面重合导致Z-fighting
            // z稍微靠后（z=2.5），高度与书柜中心对齐（y=-2.90）
            // 顶点定义中正方形已经在YZ平面（X=0），法线指向-X方向
            // 旋转：以z值较小的竖边（z=-0.5，沿Y方向）为轴，绕Y轴旋转，向室内旋转90度
            glm::vec3 brickSquarePos(3.94f, -2.20f, 2.2f); // 砖墙位置
            float brickSquareHalfSize = 0.5f; // 砖墙大小的一半（1.0/2）
            model = glm::mat4(1.0f);
            model = glm::translate(model, brickSquarePos); // 平移到砖墙位置
            // 平移到旋转轴位置（z值较小的竖边，z=-0.5）
            model = glm::translate(model, glm::vec3(0.0f, 0.0f, -brickSquareHalfSize));
            // 应用旋转：绕Y轴旋转，向室内旋转（正角度，让砖墙向室内方向旋转）
            float rotationAngle = brickSquareRotationProgress * BRICK_SQUARE_ROTATION_ANGLE;
            model = glm::rotate(model, glm::radians(-rotationAngle), glm::vec3(0.0f, 1.0f, 0.0f));
            // 平移回砖墙中心
            model = glm::translate(model, glm::vec3(0.0f, 0.0f, brickSquareHalfSize));
            lightingShader.setMat4("model", model);

            // 渲染砖墙正方形
            glBindVertexArray(brickSquareVAO);
            glDrawArrays(GL_TRIANGLES, 0, 6);
            glBindVertexArray(0);
        }

        // --- 绘制箭头系统 ---
        if (arrowSystem) {
            arrowSystem->Draw(view, projection, lightPos, camera.Position, finalLightColor);
        }
        if (windowArrowSystem) {
            windowArrowSystem->Draw(view, projection, lightPos, camera.Position, finalLightColor);
        }

        // --- 绘制书柜按钮（已注释，改用马的旋转触发） ---
        // {
        //     lightingShader.use();
        //     lightingShader.setMat4("projection", projection);
        //     lightingShader.setMat4("view", view);
        //     lightingShader.setVec3("lightPos", lightPos);
        //     lightingShader.setVec3("viewPos", camera.Position);
        //     lightingShader.setVec3("lightColor", finalLightColor);
        //     // 设置台灯点光源
        //     glm::vec3 lampLightPos = lampModelWorldPos + glm::vec3(0.0f, 0.4f, 0.0f);
        //     glm::vec3 lampColor(1.0f, 0.9f, 0.7f);
        //     lightingShader.setBool("lampOn", lampOn);
        //     lightingShader.setVec3("lampLight.position", lampLightPos);
        //     lightingShader.setVec3("lampLight.color", lampColor);
        //     lightingShader.setFloat("lampLight.intensity", lampOn ? 4.0f : 0.0f);
        //     
        //     lightingShader.setVec3("objectColor", 0.8f, 0.2f, 0.2f); // 红色按钮
        //     lightingShader.setBool("useTexture", false); // 不使用纹理
        //     lightingShader.setBool("isFloor", false);
        //     lightingShader.setBool("isHole", false);
        //     lightingShader.setBool("excludeBrickSquareRegion", false);
        //
        //     // 设置模型变换
        //     // 按钮位置：在书柜上方，右墙内表面
        //     model = glm::mat4(1.0f);
        //     model = glm::translate(model, buttonPosition); // 按钮位置（书柜上方）
        //     // 不需要旋转，因为顶点已经在YZ平面，法线已经指向-X方向
        //     lightingShader.setMat4("model", model);
        //
        //     // 渲染按钮
        //     glBindVertexArray(buttonVAO);
        //     glDrawArrays(GL_TRIANGLES, 0, 36); // 6个面，每个面6个顶点
        //     glBindVertexArray(0);
        // }

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
            lightingShader.use();
            lightingShader.setVec3("objectColor", 1.0f, 1.0f, 1.0f); // 使用纹理时设为白色
            lightingShader.setBool("useTexture", true);
            lightingShader.setBool("isFloor", false); // 不是地板
            lightingShader.setBool("isHole", false); // 不是无盖长方体
            lightingShader.setBool("excludeBrickSquareRegion", false); // 不是右墙，不需要排除砖墙区域

            // 绑定木纹纹理
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, woodTexture);
            lightingShader.setInt("floorTexture", 0);

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
            lightingShader.setBool("useTexture", false); // 窗户不使用纹理
            lightingShader.setBool("isHole", false); // 不是无盖长方体
            lightingShader.setBool("excludeBrickSquareRegion", false); // 不是右墙，不需要排除砖墙区域
            lightingShader.setVec3("objectColor", 0.36, 0.2f, 0.09f); // 木头颜色

            // 设置模型变换
            model = glm::mat4(1.0f);
            model = glm::translate(model, glm::vec3(0.0f, 0.0f, -4.0f)); // 放在后墙前一点
            model = glm::scale(model, glm::vec3(1.2f));
            lightingShader.setMat4("model", model);
            glBindVertexArray(windowVAO);
            glDrawArrays(GL_TRIANGLES, 0, windowVertexCount); 
        }

        // // 绘制房顶灯
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

            // 设置房顶灯
            modelShader.setVec3("viewPos", camera.Position);
            modelShader.setVec3("light.position", lightPos); 
            modelShader.setVec3("light.ambient", finalLightColor * 0.2f);
            modelShader.setVec3("light.diffuse", finalLightColor);
            modelShader.setVec3("light.specular", finalLightColor * 0.2f);

            // 设置台灯点光源
            // 台灯灯光位置 在灯模型中心上方一些
            glm::vec3 lampLightPos = lampModelWorldPos + glm::vec3(0.0f, 0.4f, 0.0f);
            glm::vec3 lampColor(1.0f, 0.9f, 0.7f); // 略偏暖黄的颜色
            modelShader.setBool("lampOn", lampOn);
            modelShader.setVec3("lampLight.position", lampLightPos);
            modelShader.setVec3("lampLight.color",    lampColor);
            modelShader.setFloat("lampLight.intensity", lampOn ? 4.0f : 0.0f);
            modelShader.setBool("isLampModel", false);

            // 渲染书桌模型
            model = glm::mat4(1.0f);
            model = glm::translate(model, sceneOrigin + glm::vec3(0.0f, -3.0f, 0.0f));
            model = glm::scale(model, glm::vec3(0.5f, 0.5f, 0.7f));	// 书桌模型
            model = glm::rotate(model, glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f)); // 旋转90度，使桌子朝前
            modelShader.setMat4("model", model);
            ourModel.Draw(modelShader);
        }

        // --- 绘制台灯模型 ---
        {
            // 设置台灯点光源的 uniform
            glm::vec3 lampLightPos = lampModelWorldPos + glm::vec3(0.0f, 0.4f, 0.0f);
            glm::vec3 lampColor(1.0f, 0.9f, 0.7f);
            modelShader.setBool("lampOn", lampOn);
            modelShader.setVec3("lampLight.position", lampLightPos);
            modelShader.setVec3("lampLight.color",    lampColor);
            modelShader.setFloat("lampLight.intensity", lampOn ? 4.0f : 0.0f);
            modelShader.setBool("isLampModel", true);
            // 为台灯设置一个新的 model 矩阵
            modelShader.use(); 
            model = glm::mat4(1.0f);
            // 把台灯移动到桌子上的一个偏左位置
            lampModelWorldPos = sceneOrigin + glm::vec3(-1.0f, -0.8f, -0.5f);
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
            sandboxShader.setVec3("lightPos", lightPos); 
            sandboxShader.setVec3("lightColor", finalLightColor);

            // 设置台灯点光源(世界空间位置, 由顶点着色器转换到切线空间)
            sandboxShader.setVec3("lampLightPos", lampLightPos);
            sandboxShader.setBool("lampOn", lampOn);
            sandboxShader.setVec3("lampLight.color", lampColor);
            sandboxShader.setFloat("lampLight.intensity", lampOn ? 4.0f : 0.0f);    

            model = glm::mat4(1.0f);
            // 将沙盘放在书桌上
            model = glm::translate(model, sceneOrigin + glm::vec3(0.5f, -1.4f, -1.0f)); 
            sandboxShader.setMat4("model", model);

            // 绘制沙盘 (根据选择的方法)
            sandbox_heightmap.Draw(sandboxShader);
            // sandbox_procedural.Draw(sandboxShader);
        }

        // --- 初始化马雕像位置（只在第一次运行时） ---
        static bool horsesInitialized = false;
        if (!horsesInitialized && horseModel) {
            horse1Position = sceneOrigin + glm::vec3(-3.0f, -2.95f, -3.0f);
            horse2Position = sceneOrigin + glm::vec3(3.0f, -2.95f, -3.0f);
            horse1RotationY = 90.0f;
            horse1RotationX = -90.0f;
            horse2RotationY = -90.0f;
            horse2RotationX = -90.0f;
            // 同步设置初始旋转角度（用于旋转触发检测）
            horse1InitialRotationY = horse1RotationY;
            horse2InitialRotationY = horse2RotationY;
            horsesInitialized = true;
        }

        // --- 绘制马雕像模型（放在房间左后角） ---
        if (horseModel) {
            modelShader.use();
            modelShader.setMat4("projection", projection);
            modelShader.setMat4("view", view);

            // 设置房顶灯
            modelShader.setVec3("viewPos", camera.Position);
            modelShader.setVec3("light.position", lightPos); 
            modelShader.setVec3("light.ambient", finalLightColor * 0.2f);
            modelShader.setVec3("light.diffuse", finalLightColor);
            modelShader.setVec3("light.specular", finalLightColor * 0.2f);

            // 设置台灯点光源
            glm::vec3 lampLightPos = lampModelWorldPos + glm::vec3(0.0f, 0.4f, 0.0f);
            glm::vec3 lampColor(1.0f, 0.9f, 0.7f);
            modelShader.setBool("lampOn", lampOn);
            modelShader.setVec3("lampLight.position", lampLightPos);
            modelShader.setVec3("lampLight.color", lampColor);
            modelShader.setFloat("lampLight.intensity", lampOn ? 4.0f : 0.0f);
            modelShader.setBool("isLampModel", false);

            // 绘制第一个马雕像模型 - 使用动态位置和旋转
            model = glm::mat4(1.0f);
            model = glm::translate(model, horse1Position);
            model = glm::scale(model, glm::vec3(10.0f));
            model = glm::rotate(model, glm::radians(horse1RotationY), glm::vec3(0.0f, 1.0f, 0.0f));
            model = glm::rotate(model, glm::radians(horse1RotationX), glm::vec3(1.0f, 0.0f, 0.0f));
            model = glm::rotate(model, glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f)); // 保持Z轴旋转
            modelShader.setMat4("model", model);
            horseModel->Draw(modelShader);

            // 绘制第二个马雕像模型 - 使用动态位置和旋转
            model = glm::mat4(1.0f);
            model = glm::translate(model, horse2Position);
            model = glm::scale(model, glm::vec3(10.0f));
            model = glm::rotate(model, glm::radians(horse2RotationY), glm::vec3(0.0f, 1.0f, 0.0f));
            model = glm::rotate(model, glm::radians(horse2RotationX), glm::vec3(1.0f, 0.0f, 0.0f));
            model = glm::rotate(model, glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f)); // 保持Z轴旋转ww
            modelShader.setMat4("model", model);
            horseModel->Draw(modelShader);
        }

        // --- 绘制书架模型（紧靠右墙） ---
        if (shelfModel) {
            modelShader.use();
            modelShader.setMat4("projection", projection);
            modelShader.setMat4("view", view);

            // 设置房顶灯
            modelShader.setVec3("viewPos", camera.Position);
            modelShader.setVec3("light.position", lightPos); 
            modelShader.setVec3("light.ambient", finalLightColor * 0.2f);
            modelShader.setVec3("light.diffuse", finalLightColor);
            modelShader.setVec3("light.specular", finalLightColor * 0.2f);

            // 设置台灯点光源
            glm::vec3 lampLightPos = lampModelWorldPos + glm::vec3(0.0f, 0.4f, 0.0f);
            glm::vec3 lampColor(1.0f, 0.9f, 0.7f);
            modelShader.setBool("lampOn", lampOn);
            modelShader.setVec3("lampLight.position", lampLightPos);
            modelShader.setVec3("lampLight.color", lampColor);
            modelShader.setFloat("lampLight.intensity", lampOn ? 4.0f : 0.0f);
            modelShader.setBool("isLampModel", false);

            // 设置书架位置和变换
            // 右墙在x=4.0，厚度0.1，内表面在x=3.95左右
            // 书架放在x=3.9，紧靠右墙，面向房间内部（面向负X方向，旋转180度）
            // 使用动态位置，支持向左平移
            model = glm::mat4(1.0f);
            model = glm::translate(model, shelfPosition); // 使用动态位置
            model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(0.0f, 1.0f, 0.0f)); // 面向房间内部
            model = glm::rotate(model, glm::radians(0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
            model = glm::rotate(model, glm::radians(0.0f), glm::vec3(1.0f, 0.0f, 0.0f));
            model = glm::scale(model, glm::vec3(0.2f)); // 根据模型大小调整缩放
            modelShader.setMat4("model", model);
            shelfModel->Draw(modelShader);
        }

        // --- 绘制灵珠（只在木板翘起且未拾取时渲染） ---
        if (orbShader && isFloorTileLifting && floorLiftProgress > 0.0f && !orbPicked) {
            orbShader->use();
            orbShader->setMat4("projection", projection);
            orbShader->setMat4("view", view);
            
            // 设置光源
            orbShader->setVec3("viewPos", camera.Position);
            orbShader->setVec3("lightPos", lightPos);
            orbShader->setVec3("lightColor", finalLightColor);
            
            // 设置台灯点光源
            glm::vec3 lampLightPos = lampModelWorldPos + glm::vec3(0.0f, 0.4f, 0.0f);
            glm::vec3 lampColor(1.0f, 0.9f, 0.7f);
            orbShader->setBool("lampOn", lampOn);
            orbShader->setVec3("lampLight.position", lampLightPos);
            orbShader->setVec3("lampLight.color", lampColor);
            orbShader->setFloat("lampLight.intensity", lampOn ? 4.0f : 0.0f);
            
            // 设置时间（用于颜色变化）
            orbShader->setFloat("time", currentFrame);
            
            // 设置模型变换（放在无盖长方体里面）
            // 无盖长方体中心位置：X=g_tileWorldCenterX, Y=g_holeCenterY, Z=g_tileWorldCenterZ（与翘起地板块对齐）
            model = glm::mat4(1.0f);
            model = glm::translate(model, glm::vec3(g_tileWorldCenterX, g_holeCenterY, g_tileWorldCenterZ));
            orbShader->setMat4("model", model);
            
            // 渲染球体
            glBindVertexArray(sphereVAO);
            glDrawElements(GL_TRIANGLES, sphereIndexCount, GL_UNSIGNED_INT, 0);
            glBindVertexArray(0);
        }

        // 绘制雨云 (如果可见) 
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

            // 绑定云纹理
            glActiveTexture(GL_TEXTURE0);
            cloudShader.setInt("cloudTexture", 0);
            glBindTexture(GL_TEXTURE_2D, cloudTexture);

            // 使用分层渲染来增加云的厚度
            const int numCloudLayers = 10; // 定义云的层数
            const float layerSpacing = 0.01f; // 定义每层之间的间距

            // 计算云的基础位置和缩放
            glm::vec3 sandboxBasePos = sceneOrigin + glm::vec3(0.5f, -1.4f, -0.5f);
            glm::mat4 baseModel = glm::mat4(1.0f);
            baseModel = glm::translate(baseModel, sandboxWorldPos + cloudPositionOffset);
            baseModel = glm::scale(baseModel, glm::vec3(0.5f)); // 调整云的大小
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

            // 绘制雷电
            if (lightning) {
                lightning->Draw(view, projection, camera.Position, cloudWorldCenter, sandboxWorldPos);
            }

            // 更新和绘制粒子
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
        
        // --- 检测是否靠近马雕像并显示E键提示 ---
        static bool lastShowEPrompt = false;
        bool showEPrompt = false;
        glm::vec3 eLetterPos(0.0f);
        if (!isControllingHorse && horseModel) {
            glm::vec3 playerPos = camera.Position;
            bool nearHorse1 = isNearHorse(playerPos, horse1Position, INTERACTION_DISTANCE);
            bool nearHorse2 = isNearHorse(playerPos, horse2Position, INTERACTION_DISTANCE);
            showEPrompt = nearHorse1 || nearHorse2;
            
            // 确定E字位置（在模型前方，朝向相机方向）
            glm::vec3 horsePos = nearHorse1 ? horse1Position : horse2Position;
            glm::vec3 toCamera = glm::normalize(camera.Position - horsePos);
            // 将E字放在模型前方，距离更远一些，避免被遮挡
            eLetterPos = horsePos + toCamera * 1.2f + glm::vec3(0.0f, 1.0f, 0.0f);
            
            // 只在状态改变时输出提示（避免重复输出）
            if (showEPrompt && !lastShowEPrompt) {
                std::cout << "按 E 键进入控制模式" << std::endl;
            } else if (!showEPrompt && lastShowEPrompt) {
                std::cout << "离开交互范围" << std::endl;
            }
            lastShowEPrompt = showEPrompt;
        } else {
            lastShowEPrompt = false;
        }
        
        // --- 检测是否靠近灵珠并显示E键提示（只在未拾取且木板翘起时） ---
        static bool lastShowOrbEPrompt = false;
        bool showOrbEPrompt = false;
        glm::vec3 orbELetterPos(0.0f);
        if (!orbPicked && isFloorTileLifting && floorLiftProgress > 0.0f && !isControllingHorse) {
            glm::vec3 playerPos = camera.Position;
            glm::vec3 orbPos = glm::vec3(g_tileWorldCenterX, g_holeCenterY, g_tileWorldCenterZ);
            float distanceToOrb = glm::length(playerPos - orbPos);
            showOrbEPrompt = distanceToOrb < INTERACTION_DISTANCE;
            
            if (showOrbEPrompt) {
                // 确定E字位置（在灵珠上方）
                orbELetterPos = orbPos + glm::vec3(0.0f, 0.5f, 0.0f);
            }
            lastShowOrbEPrompt = showOrbEPrompt;
        } else {
            lastShowOrbEPrompt = false;
        }
        
        // --- 检测是否靠近书柜按钮并显示E键提示（已注释，改用马的旋转触发） ---
        // static bool lastShowButtonEPrompt = false;
        // showButtonEPrompt = false;
        // if (!buttonPressed && !isControllingHorse) {
        //     glm::vec3 playerPos = camera.Position;
        //     float distanceToButton = glm::length(playerPos - buttonPosition);
        //     showButtonEPrompt = distanceToButton < INTERACTION_DISTANCE;
        //     
        //     if (showButtonEPrompt) {
        //         // 确定E字位置（在按钮上方）
        //         buttonELetterPos = buttonPosition + glm::vec3(0.0f, 0.3f, 0.0f);
        //     }
        //     lastShowButtonEPrompt = showButtonEPrompt;
        // } else {
        //     lastShowButtonEPrompt = false;
        // }
        
        // --- 检测马的旋转角度，触发动画序列 ---
        // 计算动画序列是否正在执行（任意一个动画状态为true即表示动画序列正在执行）
        isAnimationSequenceActive = shelfMoving || shelfMovingBack || brickSquareRotating || brickSquareRotatingBack || (brickSquareRotationProgress >= 1.0f && brickSquareRestTime < BRICK_SQUARE_REST_DURATION);
        
        // 只在动画序列未执行时记录旋转角度
        if (!isAnimationSequenceActive) {
            // 计算马1从初始角度转过的度数（处理角度环绕）
            float horse1CurrentRotation = horse1RotationY;
            float horse1Diff = horse1CurrentRotation - horse1InitialRotationY;
            // 将角度差归一化到[-180, 180]范围
            while (horse1Diff > 180.0f) horse1Diff -= 360.0f;
            while (horse1Diff < -180.0f) horse1Diff += 360.0f;
            horse1AccumulatedRotation = glm::abs(horse1Diff);
            
            // 计算马2从初始角度转过的度数（处理角度环绕）
            float horse2CurrentRotation = horse2RotationY;
            float horse2Diff = horse2CurrentRotation - horse2InitialRotationY;
            // 将角度差归一化到[-180, 180]范围
            while (horse2Diff > 180.0f) horse2Diff -= 360.0f;
            while (horse2Diff < -180.0f) horse2Diff += 360.0f;
            horse2AccumulatedRotation = glm::abs(horse2Diff);
            
            // 检查两个马的旋转是否都超过阈值
            if (horse1AccumulatedRotation >= HORSE_ROTATION_THRESHOLD && horse2AccumulatedRotation >= HORSE_ROTATION_THRESHOLD) {
                // 触发动画序列（与原先按下按钮相同的逻辑）
                shelfMoving = true;
                shelfMoveProgress = 0.0f;
                std::cout << "两只马都旋转超过45度，触发动画序列！书柜开始移动" << std::endl;
            }
        }
        
        // --- 更新书柜移动动画 ---
        if (shelfMoving) {
            shelfMoveProgress = glm::min(1.0f, shelfMoveProgress + SHELF_MOVE_SPEED * deltaTime);
            // 计算新的书柜位置（向z轴负方向平移，即向后移动）
            float moveAmount = shelfMoveProgress * SHELF_MOVE_DISTANCE;
            shelfPosition.z = SHELF_INITIAL_Z - moveAmount; // 向z轴负方向平移（Z减小，向后移动）
            if (shelfMoveProgress >= 1.0f) {
                shelfMoving = false;
                // 书柜移动完成后，触发砖墙旋转
                if (!brickSquareRotating && !brickSquareRotatingBack) {
                    brickSquareRotating = true;
                    brickSquareRotationProgress = 0.0f;
                    std::cout << "书柜移动完成，砖墙开始旋转" << std::endl;
                }
            }
        }
        
        // --- 更新书柜反向移动动画 ---
        if (shelfMovingBack) {
            shelfMoveProgress = glm::max(0.0f, shelfMoveProgress - SHELF_MOVE_SPEED * deltaTime);
            // 计算新的书柜位置（向z轴正方向平移，即向前移动回原位置）
            float moveAmount = shelfMoveProgress * SHELF_MOVE_DISTANCE;
            shelfPosition.z = SHELF_INITIAL_Z - moveAmount; // 向z轴正方向平移回原位置
            if (shelfMoveProgress <= 0.0f) {
                shelfMovingBack = false;
                shelfMoveProgress = 0.0f;
                shelfPosition.z = SHELF_INITIAL_Z; // 确保回到初始位置
                // 书柜反向移动完成后，重置马的累计旋转度数
                horse1AccumulatedRotation = 0.0f;
                horse2AccumulatedRotation = 0.0f;
                // 更新初始角度为当前角度，以便重新开始累计
                horse1InitialRotationY = horse1RotationY;
                horse2InitialRotationY = horse2RotationY;
                std::cout << "书柜回到原位置，动画序列结束，重置马的累计旋转度数" << std::endl;
            }
        }
        
        // --- 更新砖墙旋转动画 ---
        if (brickSquareRotating) {
            brickSquareRotationProgress = glm::min(1.0f, brickSquareRotationProgress + BRICK_SQUARE_ROTATION_SPEED * deltaTime);
            if (brickSquareRotationProgress >= 1.0f) {
                brickSquareRotating = false;
                brickSquareRotationProgress = 1.0f; // 保持最终角度
                brickSquareRestTime = 0.0f; // 开始计时静止时间
                std::cout << "砖墙旋转完成，开始静止2秒" << std::endl;
                // 砖墙旋转完成，开始发射箭头（砖墙处和窗外同时开始）
                if (arrowSystem) {
                    arrowSystem->StartShooting();
                    std::cout << "开始发射箭头（砖墙处）" << std::endl;
                }
                if (windowArrowSystem) {
                    windowArrowSystem->StartShooting();
                    std::cout << "开始发射箭头（窗外）" << std::endl;
                }
            }
        }
        
        // --- 更新砖墙静止时间 ---
        if (!brickSquareRotating && !brickSquareRotatingBack && brickSquareRotationProgress >= 1.0f) {
            brickSquareRestTime += deltaTime;
            if (brickSquareRestTime >= BRICK_SQUARE_REST_DURATION) {
                // 静止时间结束，开始反向旋转
                brickSquareRotatingBack = true;
                brickSquareRestTime = 0.0f;
                std::cout << "静止时间结束，砖墙开始反向旋转" << std::endl;
                // 砖墙开始恢复，停止发射箭头（砖墙处和窗外同时停止）
                if (arrowSystem) {
                    arrowSystem->StopShooting();
                    std::cout << "停止发射箭头（砖墙处）" << std::endl;
                }
                if (windowArrowSystem) {
                    windowArrowSystem->StopShooting();
                    std::cout << "停止发射箭头（窗外）" << std::endl;
                }
            }
        }
        
        // --- 更新砖墙反向旋转动画 ---
        if (brickSquareRotatingBack) {
            brickSquareRotationProgress = glm::max(0.0f, brickSquareRotationProgress - BRICK_SQUARE_ROTATION_SPEED * deltaTime);
            if (brickSquareRotationProgress <= 0.0f) {
                brickSquareRotatingBack = false;
                brickSquareRotationProgress = 0.0f; // 确保回到初始角度
                // 砖墙反向旋转完成后，开始书柜反向移动
                shelfMovingBack = true;
                std::cout << "砖墙反向旋转完成，书柜开始反向移动" << std::endl;
            }
        }
        
        // --- 更新箭头系统 ---
        if (arrowSystem) {
            arrowSystem->Update(deltaTime);
        }
        if (windowArrowSystem) {
            windowArrowSystem->Update(deltaTime);
        }
        
        // --- 渲染"E"字提示（世界空间，billboard，在HDR FBO中） ---
        if (showEPrompt && billboardShader && horseModel) {
            // 确保在HDR FBO中渲染
            glBindFramebuffer(GL_FRAMEBUFFER, hdrFBO);
            
            // 启用混合以支持透明度
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            // 禁用深度测试，确保E字始终显示在最前面
            glDisable(GL_DEPTH_TEST);
            
            billboardShader->use();
            billboardShader->setMat4("projection", projection);
            billboardShader->setMat4("view", view);
            billboardShader->setVec3("textColor", glm::vec3(1.0f, 1.0f, 0.0f)); // 黄色
            
            // 设置billboard位置和大小（缩小一些）
            billboardShader->setVec3("centerPos", eLetterPos);
            billboardShader->setVec2("size", glm::vec2(0.2f, 0.3f)); 
            
            glBindVertexArray(eVAO);
            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
            glBindVertexArray(0);
            
            // 恢复状态
            glEnable(GL_DEPTH_TEST);
            glDisable(GL_BLEND);
        }
        
        // --- 渲染灵珠的"E"字提示（世界空间，billboard，在HDR FBO中） ---
        if (showOrbEPrompt && billboardShader) {
            // 确保在HDR FBO中渲染
            glBindFramebuffer(GL_FRAMEBUFFER, hdrFBO);
            
            // 启用混合以支持透明度
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            // 禁用深度测试，确保E字始终显示在最前面
            glDisable(GL_DEPTH_TEST);
            
            billboardShader->use();
            billboardShader->setMat4("projection", projection);
            billboardShader->setMat4("view", view);
            billboardShader->setVec3("textColor", glm::vec3(1.0f, 1.0f, 0.0f)); // 黄色
            
            // 设置billboard位置和大小（缩小一些）
            billboardShader->setVec3("centerPos", orbELetterPos);
            billboardShader->setVec2("size", glm::vec2(0.2f, 0.3f)); 
            
            glBindVertexArray(eVAO);
            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
            glBindVertexArray(0);
            
            // 恢复状态
            glEnable(GL_DEPTH_TEST);
            glDisable(GL_BLEND);
        }
        
        // --- 渲染按钮的"E"字提示（已注释，改用马的旋转触发） ---
        // if (showButtonEPrompt && billboardShader) {
        //     // 确保在HDR FBO中渲染
        //     glBindFramebuffer(GL_FRAMEBUFFER, hdrFBO);
        //     
        //     // 启用混合以支持透明度
        //     glEnable(GL_BLEND);
        //     glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        //     // 禁用深度测试，确保E字始终显示在最前面
        //     glDisable(GL_DEPTH_TEST);
        //     
        //     billboardShader->use();
        //     billboardShader->setMat4("projection", projection);
        //     billboardShader->setMat4("view", view);
        //     billboardShader->setVec3("textColor", glm::vec3(1.0f, 1.0f, 0.0f)); // 黄色
        //     
        //     // 设置billboard位置和大小（缩小一些）
        //     billboardShader->setVec3("centerPos", buttonELetterPos);
        //     billboardShader->setVec2("size", glm::vec2(0.2f, 0.3f)); 
        //     
        //     glBindVertexArray(eVAO);
        //     glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
        //     glBindVertexArray(0);
        //     
        //     // 恢复状态
        //     glEnable(GL_DEPTH_TEST);
        //     glDisable(GL_BLEND);
        // }
        
        // 提取超过亮度阈值的区域
        glBindFramebuffer(GL_FRAMEBUFFER, pingpongFBO[0]);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        bloomThresholdShader->use();
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, colorBuffers[0]);  // 使用第一个颜色缓冲（主场景）
        bloomThresholdShader->setInt("sceneTexture", 0);
        bloomThresholdShader->setFloat("threshold", 0.8f);  // 阈值

        glBindVertexArray(quadVAO);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        glBindVertexArray(0);

        // 高斯模糊（多次迭代）
        bool horizontal = true, first_iteration = true;
        bloomBlurShader->use();
        for (unsigned int i = 0; i < bloomAmount; i++)
        {
            glBindFramebuffer(GL_FRAMEBUFFER, pingpongFBO[horizontal]);
            bloomBlurShader->setBool("horizontal", horizontal);
            bloomBlurShader->setFloat("blurSize", 1.0f);
            
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, first_iteration ? pingpongColorBuffers[0] : pingpongColorBuffers[!horizontal]);
            bloomBlurShader->setInt("image", 0);
            
            glBindVertexArray(quadVAO);
            glDrawArrays(GL_TRIANGLES, 0, 6);
            glBindVertexArray(0);
            
            horizontal = !horizontal;
            if (first_iteration)
                first_iteration = false;
        }
        // 将 Bloom 效果混合到最终场景
        glBindFramebuffer(GL_FRAMEBUFFER, 0);  // 绑定到默认帧缓冲
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        bloomFinalShader->use();
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, colorBuffers[0]);  // 原始场景
        bloomFinalShader->setInt("sceneTexture", 0);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, pingpongColorBuffers[!horizontal]);  // 模糊后的Bloom纹理
        bloomFinalShader->setInt("bloomTexture", 1);
        bloomFinalShader->setFloat("bloomStrength", 0.8f);  // Bloom强度

        glBindVertexArray(quadVAO);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        glBindVertexArray(0);

        // --- 在屏幕中心显示拾取消息（在最终渲染后） ---
        if (showOrbPickupMessage && textDisplayShader) {
            // 确保在默认帧缓冲上渲染
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            
            // 启用混合以支持透明度
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            // 禁用深度测试
            glDisable(GL_DEPTH_TEST);
            
            textDisplayShader->use();
            textDisplayShader->setVec2("screenSize", glm::vec2(SCR_WIDTH, SCR_HEIGHT));
            // 根据剩余时间计算透明度（淡入淡出效果）
            float fadeTime = 0.5f; // 淡入淡出时间（秒）
            float alpha = 1.0f;
            if (orbPickupMessageTime < fadeTime) {
                alpha = orbPickupMessageTime / fadeTime; // 淡入
            } else if (orbPickupMessageTime > ORB_PICKUP_MESSAGE_DURATION - fadeTime) {
                alpha = (ORB_PICKUP_MESSAGE_DURATION - orbPickupMessageTime) / fadeTime; // 淡出
            }
            textDisplayShader->setFloat("alpha", alpha * 0.9f); // 稍微透明
            
            // 绑定消息纹理
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, messageTexture);
            textDisplayShader->setInt("textTexture", 0);
            
            glBindVertexArray(messageQuadVAO);
            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
            glBindVertexArray(0);
            
            // 恢复状态
            glEnable(GL_DEPTH_TEST);
            glDisable(GL_BLEND);
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
    glDeleteVertexArrays(1, &cloudVAO);
    glDeleteVertexArrays(1, &floorVAO);
    glDeleteVertexArrays(1, &liftedTileVAO);
    glDeleteVertexArrays(1, &liftedTileHoleVAO);
    glDeleteVertexArrays(1, &brickSquareVAO);
    glDeleteVertexArrays(1, &buttonVAO);
    glDeleteVertexArrays(1, &brickTile1VAO);
    glDeleteVertexArrays(1, &brickTile2VAO);
    
    glDeleteBuffers(1, &roomVBO);
    glDeleteBuffers(1, &windowVBO);
    glDeleteBuffers(1, &wallVBO);
    glDeleteBuffers(1, &cloudVBO); 
    glDeleteBuffers(1, &cloudEBO);
    glDeleteBuffers(1, &floorVBO);
    glDeleteBuffers(1, &liftedTileVBO);
    glDeleteBuffers(1, &liftedTileHoleVBO);
    glDeleteBuffers(1, &brickSquareVBO);
    glDeleteBuffers(1, &buttonVBO);
    glDeleteBuffers(1, &brickTile1VBO);
    glDeleteBuffers(1, &brickTile2VBO);
    
    glDeleteTextures(1, &floorTexture);
    glDeleteTextures(1, &woodTexture);
    glDeleteTextures(1, &brickTexture);
    glDeleteTextures(1, &brickWallTexture);
    glDeleteTextures(1, &messageTexture);

    // 清理 Bloom 资源
    glDeleteFramebuffers(1, &hdrFBO);
    glDeleteFramebuffers(2, pingpongFBO);
    glDeleteTextures(2, colorBuffers);
    glDeleteTextures(2, pingpongColorBuffers);
    glDeleteRenderbuffers(1, &rboDepth);
    glDeleteVertexArrays(1, &quadVAO);
    glDeleteBuffers(1, &quadVBO);
    if (bloomThresholdShader) delete bloomThresholdShader;
    if (bloomBlurShader) delete bloomBlurShader;
    if (bloomFinalShader) delete bloomFinalShader;

    // 清理雷电资源 
    if (lightning) delete lightning;
    if (lightningShader) delete lightningShader;

    // 清理马雕像模型
    if (horseModel) delete horseModel;
    if (shelfModel) delete shelfModel;
    if (arrowModel) delete arrowModel;
    if (arrowSystem) delete arrowSystem;
    if (windowArrowSystem) delete windowArrowSystem;

    // 清理文字显示资源
    glDeleteVertexArrays(1, &eVAO);
    glDeleteBuffers(1, &eVBO);
    glDeleteBuffers(1, &eEBO);
    glDeleteVertexArrays(1, &messageQuadVAO);
    glDeleteBuffers(1, &messageQuadVBO);
    glDeleteBuffers(1, &messageQuadEBO);
    if (textDisplayShader) delete textDisplayShader;
    if (billboardShader) delete billboardShader;
    if (orbShader) delete orbShader;
    glDeleteVertexArrays(1, &sphereVAO);
    glDeleteBuffers(1, &sphereVBO);
    glDeleteBuffers(1, &sphereEBO);

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

    // E键处理：进入/退出控制模式 或 拾取灵珠
    if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS && !eKeyPressed) {
        eKeyPressed = true;
        if (isControllingHorse) {
            // 退出控制模式：恢复相机状态
            isControllingHorse = false;
            camera.Position = savedCameraPosition;
            camera.Yaw = savedCameraYaw;
            camera.Pitch = savedCameraPitch;
            camera.ProcessMouseMovement(0.0f, 0.0f); // 触发向量更新
            std::cout << "退出控制模式" << std::endl;
        } else {
            glm::vec3 playerPos = camera.Position;
            bool nearHorse1 = isNearHorse(playerPos, horse1Position, INTERACTION_DISTANCE);
            bool nearHorse2 = isNearHorse(playerPos, horse2Position, INTERACTION_DISTANCE);
            
            // 优先检查是否靠近马雕像（如果靠近马，就不检查灵珠）
            if (nearHorse1) {
                // 保存当前相机状态（用于退出时恢复）
                savedCameraPosition = camera.Position;
                savedCameraYaw = camera.Yaw;
                savedCameraPitch = camera.Pitch;
                
                // 进入控制模式：将相机设置为程序启动时的初始视角
                camera.Position = initialCameraPosition;
                camera.Yaw = initialCameraYaw;
                camera.Pitch = initialCameraPitch;
                camera.ProcessMouseMovement(0.0f, 0.0f); // 触发向量更新
                
                isControllingHorse = true;
                controlledHorseIndex = 0;
                
                std::cout << "进入控制模式 - 马雕像1" << std::endl;
            } else if (nearHorse2) {
                // 保存当前相机状态（用于退出时恢复）
                savedCameraPosition = camera.Position;
                savedCameraYaw = camera.Yaw;
                savedCameraPitch = camera.Pitch;
                
                // 进入控制模式：将相机设置为程序启动时的初始视角
                camera.Position = initialCameraPosition;
                camera.Yaw = initialCameraYaw;
                camera.Pitch = initialCameraPitch;
                camera.ProcessMouseMovement(0.0f, 0.0f); // 触发向量更新
                
                isControllingHorse = true;
                controlledHorseIndex = 1;
                
                std::cout << "进入控制模式 - 马雕像2" << std::endl;
            } else {
                // 只有在不靠近马雕像的情况下，才检查其他交互
                // 按钮交互已注释，改用马的旋转触发
                // if (!buttonPressed) {
                //     float distanceToButton = glm::length(playerPos - buttonPosition);
                //     if (distanceToButton < INTERACTION_DISTANCE) {
                //         // 按下按钮，触发书柜移动
                //         buttonPressed = true;
                //         shelfMoving = true;
                //         shelfMoveProgress = 0.0f;
                //         std::cout << "按下按钮，书柜开始向左移动" << std::endl;
                //     }
                // }
                // 检查是否拾取灵珠
                if (!orbPicked && isFloorTileLifting && floorLiftProgress > 0.0f) {
                    glm::vec3 orbPos = glm::vec3(g_tileWorldCenterX, g_holeCenterY, g_tileWorldCenterZ);
                    float distanceToOrb = glm::length(playerPos - orbPos);
                    if (distanceToOrb < INTERACTION_DISTANCE) {
                        // 拾取灵珠
                        orbPicked = true;
                        showOrbPickupMessage = true;
                        orbPickupMessageTime = 0.0f;
                        std::cout << "已拾取灵珠！" << std::endl;
                    }
                }
            }
        }
    }
    if (glfwGetKey(window, GLFW_KEY_E) == GLFW_RELEASE) {
        eKeyPressed = false;
    }

    // 相机/模型移动
    if (isControllingHorse) {
        // 控制模式下：只移动模型，相机保持固定
        glm::vec3& horsePos = (controlledHorseIndex == 0) ? horse1Position : horse2Position;
        float& horseRotY = (controlledHorseIndex == 0) ? horse1RotationY : horse2RotationY;
        
        // WASD控制模型移动（基于相机视角方向）
        glm::vec3 moveDir(0.0f);
        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
            moveDir += camera.Front;
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
            moveDir -= camera.Front;
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
            moveDir -= camera.Right;
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
            moveDir += camera.Right;
        
        // 归一化移动方向，并限制在水平平面内（Y=0）
        if (glm::length(moveDir) > 0.0f) {
            moveDir = glm::normalize(moveDir);
            moveDir.y = 0.0f; // 限制在水平平面内
            moveDir = glm::normalize(moveDir);
            float moveSpeed = 2.5f * deltaTime;
            
            // 只移动模型（在水平平面内），相机位置不变
            horsePos += moveDir * moveSpeed;
            horsePos = clampToRoomBounds(horsePos);
        }
        
        // Q和R键控制模型旋转（顺时针和逆时针）
        float rotationSpeed = 90.0f * deltaTime; // 每秒90度
        if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS) {
            // Q键：顺时针旋转（Y轴增加）
            horseRotY += rotationSpeed;
        }
        if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS) {
            // R键：逆时针旋转（Y轴减少）
            horseRotY -= rotationSpeed;
        }
    } else {
        // 正常模式：只移动相机
        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
            camera.ProcessKeyboard(FORWARD, deltaTime);
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
            camera.ProcessKeyboard(BACKWARD, deltaTime);
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
            camera.ProcessKeyboard(LEFT, deltaTime);
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
            camera.ProcessKeyboard(RIGHT, deltaTime);
    }

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

    if (isControllingHorse) {
        // 控制模式下：禁用鼠标旋转，相机保持固定
        // 不处理鼠标输入
    } else {
        // 正常模式：只旋转相机
        camera.ProcessMouseMovement(xoffset, yoffset);
    }
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
    // 只在按下左键时进行处理
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS)
    {
        // 使用屏幕中心作为选取点
        float x = SCR_WIDTH * 0.5f;
        float y = SCR_HEIGHT * 0.5f;

        // 转换到标准设备坐标(NDC)
        float ndcX = 2.0f * x / SCR_WIDTH - 1.0f;
        float ndcY = 1.0f - 2.0f * y / SCR_HEIGHT; // Y轴反转
        glm::vec4 clip(ndcX, ndcY, -1.0f, 1.0f);

        // 构造当前的投影矩阵和视图矩阵
        glm::mat4 projection = glm::perspective(
            glm::radians(camera.Zoom),
            float(SCR_WIDTH) / float(SCR_HEIGHT),
            0.1f,
            100.0f
        );
        glm::mat4 view = camera.GetViewMatrix();

        glm::mat4 invVP = glm::inverse(projection * view);

        // 将NDC点反变换到世界空间
        glm::vec4 worldPos = invVP * clip;
        worldPos /= worldPos.w;

        glm::vec3 rayOrigin = camera.Position;
        glm::vec3 rayDir = glm::normalize(glm::vec3(worldPos) - rayOrigin);

        // 判断是否打到台灯
        if (rayHitLamp(rayOrigin, rayDir))
        {
            lampOn = !lampOn;
            std::cout << "台灯状态: " << (lampOn ? "打开" : "关闭") << std::endl;
        }
    }
}

bool rayHitLamp(const glm::vec3& rayOrigin, const glm::vec3& rayDir)
{
    // 使用一个球体包围台灯，判断射线是否与球体相交
    glm::vec3 center = lampModelWorldPos;
    float radius = 0.6f;

    // 射线-球求交 
    glm::vec3 oc = rayOrigin - center;

    // 求解 t^2 + 2*(oc·D)*t + (oc·oc - r^2) = 0 的判别式
    float b = glm::dot(oc, rayDir); // 等价于 0.5 * 2*(oc·D)
    float c = glm::dot(oc, oc) - radius * radius;
    float discriminant = b * b - c;

    // 无解则不相交
    if (discriminant < 0.0f) return false;

    // 确保在摄像机前方
    float t1 = -b - glm::sqrt(discriminant);
    float t2 = -b + glm::sqrt(discriminant);
    if (t1 < 0.0f && t2 < 0.0f) return false;
    
    return true;
}

// 检查玩家是否靠近马雕像
bool isNearHorse(const glm::vec3& playerPos, const glm::vec3& horsePos, float threshold)
{
    float distance = glm::length(playerPos - horsePos);
    return distance < threshold;
}

// 限制位置在房间范围内
glm::vec3 clampToRoomBounds(const glm::vec3& position)
{
    glm::vec3 clamped = position;
    // 房间范围：x: -3.5 到 3.5, y: -2.95 到 2.5, z: -3.5 到 3.5
    clamped.x = glm::clamp(clamped.x, -3.5f, 3.5f);
    clamped.y = glm::clamp(clamped.y, -2.95f, 2.5f);
    clamped.z = glm::clamp(clamped.z, -3.5f, 3.5f);
    return clamped;
}

// 检查两个马是否都在z轴尽头
bool areHorsesAtZAxisEnd()
{
    return (horse1Position.z >= Z_AXIS_THRESHOLD && horse2Position.z >= Z_AXIS_THRESHOLD);
}

// 检查马1是否在地砖1范围内
bool isHorse1OnTile1()
{
    // std::cout << "马1已到位!" << std::endl;
    // 马的scale是10.0f，假设马的底座大约是1.0-1.5单位
    // 地砖大小是2.0x2.0，足够大
    // 检查马的位置是否在地砖范围内
    return (horse1Position.x >= BRICK_TILE1_MIN_X && horse1Position.x <= BRICK_TILE1_MAX_X &&
            horse1Position.z >= BRICK_TILE1_MIN_Z && horse1Position.z <= BRICK_TILE1_MAX_Z);
}

// 检查马2是否在地砖2范围内
bool isHorse2OnTile2()
{
    // std::cout << "马2已到位!" << std::endl;
    // 马的scale是10.0f，假设马的底座大约是1.0-1.5单位
    // 地砖大小是2.0x2.0，足够大
    // 检查马的位置是否在地砖范围内
    return (horse2Position.x >= BRICK_TILE2_MIN_X && horse2Position.x <= BRICK_TILE2_MAX_X &&
            horse2Position.z >= BRICK_TILE2_MIN_Z && horse2Position.z <= BRICK_TILE2_MAX_Z);
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
    // ---------------------------------------------------------
    // 定义一个临时辅助函数来绘制任意角度的线段
    auto addLine = [&](glm::vec2 start, glm::vec2 end, float width) {
        glm::vec2 dir = glm::normalize(end - start);
        glm::vec2 perp(-dir.y, dir.x); // 计算垂直向量
        glm::vec2 offset = perp * (width * 0.5f);

        // 计算线段四个角的坐标
        glm::vec2 p1 = start + offset; // 起点外侧
        glm::vec2 p2 = start - offset; // 起点内侧
        glm::vec2 p3 = end - offset;   // 终点内侧
        glm::vec2 p4 = end + offset;   // 终点外侧

        // 使用 add3DStrip 绘制 (参数顺序: inner1, outer1, inner2, outer2)
        add3DStrip(p2, p1, p3, p4);
    };
    // 定义斜线的宽度 
    float diagonalWidth = barWidth * 1.5f;

    // 第一象限 (右上) - 旋转90度：连接 (0.4, 0.2) 和 (0.2, 0.4)
    addLine({0.45f * innerRadius, 0.15f * innerRadius}, {0.15f * innerRadius, 0.45f * innerRadius}, diagonalWidth);
    
    // 第二象限 (左上) - 旋转90度：连接 (-0.2, 0.4) 和 (-0.4, 0.2)
    addLine({-0.15f * innerRadius, 0.45f * innerRadius}, {-0.45f * innerRadius, 0.15f * innerRadius}, diagonalWidth);
    
    // 第三象限 (左下) - 旋转90度：连接 (-0.4, -0.2) 和 (-0.2, -0.4)
    addLine({-0.45 * innerRadius, -0.15f * innerRadius}, {-0.15f * innerRadius, -0.45f * innerRadius}, diagonalWidth);
    
    // 第四象限 (右下) - 旋转90度：连接 (0.2, -0.4) 和 (0.4, -0.2)
    addLine({0.15f * innerRadius, -0.45f * innerRadius}, {0.45f * innerRadius, -0.15f * innerRadius}, diagonalWidth);
    // ---------------------------------------------------------

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

        // 计算纹理坐标（基于世界坐标，归一化到0-1范围）
        float texU1 = (inner1.x + halfW) / width;
        float texV1 = (inner1.y + halfH) / height;
        float texU2 = (outer1.x + halfW) / width;
        float texV2 = (outer1.y + halfH) / height;
        float texU3 = (outer2.x + halfW) / width;
        float texV3 = (outer2.y + halfH) / height;
        float texU4 = (inner2.x + halfW) / width;
        float texV4 = (inner2.y + halfH) / height;

        // 用两个三角形构成一个四边形
        // 三角形 1
        vertices.insert(vertices.end(), {inner1.x, inner1.y, inner1.z, normal.x, normal.y, normal.z, texU1, texV1});
        vertices.insert(vertices.end(), {outer1.x, outer1.y, outer1.z, normal.x, normal.y, normal.z, texU2, texV2});
        vertices.insert(vertices.end(), {outer2.x, outer2.y, outer2.z, normal.x, normal.y, normal.z, texU3, texV3});

        // 三角形 2
        vertices.insert(vertices.end(), {inner1.x, inner1.y, inner1.z, normal.x, normal.y, normal.z, texU1, texV1});
        vertices.insert(vertices.end(), {outer2.x, outer2.y, outer2.z, normal.x, normal.y, normal.z, texU3, texV3});
        vertices.insert(vertices.end(), {inner2.x, inner2.y, inner2.z, normal.x, normal.y, normal.z, texU4, texV4});
    }

    return vertices;
}