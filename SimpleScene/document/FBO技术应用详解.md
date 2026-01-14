# 帧缓冲对象（FBO）技术应用详解

本项目在实时渲染中使用了帧缓冲对象（Framebuffer Object，FBO）技术，主要应用于两个核心场景：**地形沙盘的动态纹理绘制**和**Bloom后处理效果**。本文将结合项目代码详细讲解这两种应用。

---

## 一、地形沙盘动态纹理绘制系统

### 1.1 应用背景

在场景的中央沙盘上，当雨滴或雪花粒子碰撞到地形时，需要在地形上动态生成草地或积雪效果。传统的静态纹理无法满足这种运行时动态修改的需求，因此使用了 FBO 技术将效果实时绘制到一个专门的蒙版纹理上。

### 1.2 FBO 初始化

在 `sandbox_mesh.cpp` 的 `setupGrowthTexture()` 函数中，创建了用于绘制蒙版纹理的 FBO：

```390:409:sandbox_mesh.cpp
void TerrainSandbox::setupGrowthTexture() {
    // 创建生长纹理
    glGenTextures(1, &growthTexture);
    glBindTexture(GL_TEXTURE_2D, growthTexture);
    // 创建一个 512x512 的空白纹理，初始为黑色
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, growthTextureSize, growthTextureSize, 0, GL_RGBA, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    // 创建FBO并绑定纹理
    glGenFramebuffers(1, &growthFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, growthFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, growthTexture, 0);

    // 检查FBO是否完整
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        std::cout << "ERROR::FRAMEBUFFER:: Framebuffer is not complete!" << std::endl;
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
```

**关键技术点：**

- **纹理格式**：使用 `GL_RGBA` 格式，32位浮点精度（`GL_FLOAT`），确保有足够的精度存储累积的效果值
- **纹理尺寸**：512×512 像素，这是蒙版纹理的分辨率，与地形网格精度独立
- **FBO 绑定**：使用 `glFramebufferTexture2D()` 将纹理作为 FBO 的颜色附件，使得后续的绘制操作会直接写入这个纹理
- **完整性检查**：使用 `glCheckFramebufferStatus()` 确保 FBO 配置正确

### 1.3 动态绘制流程

当粒子碰撞到地形时，调用 `addGrowth()` 或 `addSnow()` 函数进行绘制：

```424:458:sandbox_mesh.cpp
void TerrainSandbox::addGrowth(float worldX, float worldZ) {
    const unsigned int SCR_WIDTH = 1600;
    const unsigned int SCR_HEIGHT = 1200;
    // 将世界坐标转换为地形UV坐标 (0-1范围)
    float u = (worldX / terrainWidth) + 0.5f;
    float v = (worldZ / terrainDepth) + 0.5f;

    // 如果在范围外则忽略
    if (u < 0 || u > 1 || v < 0 || v > 1) return;

    // 开始绘制到 growthTexture
    glViewport(0, 0, growthTextureSize, growthTextureSize);
    glBindFramebuffer(GL_FRAMEBUFFER, growthFBO);
    
    // 启用混合，这样每次绘制都是叠加效果
    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE); // 加法混合

    paintShader.use();
    paintShader.setVec2("center", glm::vec2(u, v));
    paintShader.setFloat("radius", 0.15f); // 草地斑块的半径
    paintShader.setFloat("scale", (float)growthTextureSize);
    // 绘制到红色通道
    paintShader.setVec3("paintColor", glm::vec3(1.0f, 0.0f, 0.0f)); 

    glBindVertexArray(paintQuadVAO);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glBindVertexArray(0);

    glDisable(GL_BLEND);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // 恢复主视口
    glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT); 
}
```

**绘制流程解析：**

1. **坐标转换**：将粒子的世界坐标转换为地形的 UV 坐标（0-1 范围）
2. **视口设置**：将视口设置为蒙版纹理的尺寸（512×512），确保绘制正确映射
3. **FBO 绑定**：绑定 `growthFBO`，后续的所有绘制操作都会写入 `growthTexture`
4. **混合模式**：使用 `GL_ONE, GL_ONE` 加法混合，使得多次绘制在同一位置会累积效果值
5. **全屏四边形绘制**：使用一个覆盖整个纹理的全屏四边形，在片段着色器中根据距离中心点的距离计算绘制强度
6. **颜色通道分配**：
   - 草地效果绘制到**红色通道**（`paintColor = (1.0, 0.0, 0.0)`）
   - 积雪效果绘制到**绿色通道**（`paintColor = (0.0, 0.5, 0.0)`）
7. **状态恢复**：恢复默认帧缓冲和主视口

### 1.4 片段着色器绘制逻辑

`paint.fs` 着色器实现了圆形笔刷效果：

```1:15:paint.fs
#version 330 core
out vec4 FragColor;

uniform vec2 center;
uniform float radius;
uniform float scale;
uniform vec3 paintColor;

void main()
{
    float dist = distance(gl_FragCoord.xy, center * scale);
    float intensity = 1.0 - smoothstep(0.0, radius, dist);
    
    FragColor = vec4(paintColor * intensity, 1.0);
}
```

- 使用 `gl_FragCoord.xy` 获取当前片段在纹理空间中的像素坐标
- 计算到中心点的距离，使用 `smoothstep()` 函数创建平滑的衰减效果
- 输出结果直接写入 FBO 绑定的纹理

### 1.5 纹理采样与混合

在 `sandbox.fs` 片段着色器中，动态蒙版纹理被采样并用于混合多种纹理：

```72:80:sandbox.fs
    vec4 sandColor = texture(texture_diffuse1, fs_in.TexCoords);
    vec4 grassColor = texture(texture_grass1, fs_in.TexCoords);
    vec4 snowColor = texture(texture_snow1, fs_in.TexCoords);
    vec4 effectMask = texture(texture_effect_mask, fs_in.TexCoords);

    float grassAmount = effectMask.r;
    float snowAmount = effectMask.g;
```

红色通道存储草地强度，绿色通道存储积雪强度，通过线性插值在基础纹理、草地纹理和雪地纹理之间混合。

---

## 二、Bloom 后处理效果系统

### 2.1 应用背景

Bloom（泛光）效果用于增强场景中发光物体的视觉表现，如台灯、灵珠等。实现 Bloom 需要多通道渲染：首先渲染场景到 HDR 纹理，提取高亮区域，进行模糊处理，最后与原始场景合成。

### 2.2 HDR FBO 初始化

在 `main.cpp` 中创建用于存储场景渲染结果的 HDR FBO：

```1323:1354:main.cpp
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
```

**关键技术点：**

- **多渲染目标（MRT）**：使用两个颜色附件（`GL_COLOR_ATTACHMENT0` 和 `GL_COLOR_ATTACHMENT1`），分别存储场景的正常渲染结果和需要 Bloom 的高亮区域
- **HDR 格式**：使用 `GL_RGB16F`（16位浮点）格式，支持超出 0-1 范围的颜色值，这对于 Bloom 效果至关重要
- **深度缓冲**：使用渲染缓冲对象（RBO）存储深度信息，确保 3D 场景的正确深度排序
- **多附件绑定**：使用 `glDrawBuffers()` 指定同时渲染到两个颜色附件

### 2.3 Ping-Pong FBO 模糊系统

为了高效实现高斯模糊，使用了 Ping-Pong 技术（双缓冲交替使用）：

```1356:1373:main.cpp
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
```

创建两个 FBO，用于交替进行水平和垂直方向的模糊。

### 2.4 Bloom 渲染管线

Bloom 效果通过以下步骤实现：

**步骤 1：渲染场景到 HDR FBO**

```1490:1492:main.cpp
        // 渲染到 HDR FBO
        glBindFramebuffer(GL_FRAMEBUFFER, hdrFBO);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
```

所有场景物体都渲染到 `hdrFBO`，结果存储在 `colorBuffers[0]` 和 `colorBuffers[1]` 中。

**步骤 2：提取高亮区域**

```2461:2473:main.cpp
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
```

使用阈值着色器提取亮度超过 0.8 的区域，绘制到第一个 Ping-Pong FBO。

**步骤 3：高斯模糊（Ping-Pong 迭代）**

```2475:2495:main.cpp
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
```

**Ping-Pong 技术解析：**

- 第一次迭代：从 `pingpongColorBuffers[0]` 读取，水平模糊，写入 `pingpongFBO[1]`
- 第二次迭代：从 `pingpongColorBuffers[1]` 读取，垂直模糊，写入 `pingpongFBO[0]`
- 重复交替，每次迭代都交替水平和垂直方向，实现 2D 高斯模糊效果

通过分离的水平/垂直模糊，将 O(n²) 的 2D 高斯模糊降低为 O(2n)，大幅提升性能。

**步骤 4：最终合成**

```2496:2511:main.cpp
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
```

绑定默认帧缓冲（屏幕），将原始场景和模糊后的高亮区域进行加法混合，得到最终的 Bloom 效果。

---

## 三、FBO 技术的优势总结

1. **离屏渲染**：可以将渲染结果存储到纹理中，供后续使用
2. **多通道渲染**：支持同时输出多个渲染结果（如 Bloom 的双颜色附件）
3. **后处理效果**：实现各种屏幕空间效果（如模糊、色调映射等）
4. **动态纹理**：运行时修改纹理内容，实现交互式效果
5. **性能优化**：通过分离的模糊步骤优化复杂效果的计算

本项目中的 FBO 应用展现了现代实时渲染中 FBO 技术的典型用法，是学习图形编程的优秀案例。

