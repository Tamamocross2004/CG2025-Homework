#version 330 core
out vec4 FragColor;

uniform vec3 textColor;
uniform float alpha;
uniform vec2 screenSize;

in vec2 fragPos;  // 片段位置（屏幕像素坐标）

// 简单的5x7点阵字体数据（用于绘制"ORB PICKED!"）
// 使用二维数组直接表示每个字符的像素

// 字符'O'的5x7点阵
int getCharO(int x, int y) {
    if (y == 0 && x >= 1 && x <= 3) return 1;
    if (y == 1 && (x == 0 || x == 4)) return 1;
    if (y == 2 && (x == 0 || x == 4)) return 1;
    if (y == 3 && (x == 0 || x == 4)) return 1;
    if (y == 4 && (x == 0 || x == 4)) return 1;
    if (y == 5 && (x == 0 || x == 4)) return 1;
    if (y == 6 && x >= 1 && x <= 3) return 1;
    return 0;
}

// 字符'R'的5x7点阵
int getCharR(int x, int y) {
    if (y == 0 && x <= 3) return 1;
    if (y == 1 && (x == 0 || x == 4)) return 1;
    if (y == 2 && (x == 0 || x == 4)) return 1;
    if (y == 3 && x <= 3) return 1;
    if (y == 4 && (x == 0 || x == 3)) return 1;
    if (y == 5 && (x == 0 || x == 4)) return 1;
    if (y == 6 && (x == 0 || x == 4)) return 1;
    return 0;
}

// 字符'B'的5x7点阵
int getCharB(int x, int y) {
    if (y == 0 && x <= 3) return 1;
    if (y == 1 && (x == 0 || x == 4)) return 1;
    if (y == 2 && (x == 0 || x == 4)) return 1;
    if (y == 3 && x <= 3) return 1;
    if (y == 4 && (x == 0 || x == 4)) return 1;
    if (y == 5 && (x == 0 || x == 4)) return 1;
    if (y == 6 && x <= 3) return 1;
    return 0;
}

// 字符'P'的5x7点阵
int getCharP(int x, int y) {
    if (y == 0 && x <= 3) return 1;
    if (y == 1 && (x == 0 || x == 4)) return 1;
    if (y == 2 && (x == 0 || x == 4)) return 1;
    if (y == 3 && x <= 3) return 1;
    if (y == 4 && x == 0) return 1;
    if (y == 5 && x == 0) return 1;
    if (y == 6 && x == 0) return 1;
    return 0;
}

// 字符'I'的5x7点阵
int getCharI(int x, int y) {
    if (y == 0 && x >= 0 && x <= 4) return 1;
    if (y == 1 && x == 2) return 1;
    if (y == 2 && x == 2) return 1;
    if (y == 3 && x == 2) return 1;
    if (y == 4 && x == 2) return 1;
    if (y == 5 && x == 2) return 1;
    if (y == 6 && x >= 0 && x <= 4) return 1;
    return 0;
}

// 字符'C'的5x7点阵
int getCharC(int x, int y) {
    if (y == 0 && x >= 1 && x <= 3) return 1;
    if (y == 1 && (x == 0 || x == 4)) return 1;
    if (y == 2 && x == 0) return 1;
    if (y == 3 && x == 0) return 1;
    if (y == 4 && x == 0) return 1;
    if (y == 5 && (x == 0 || x == 4)) return 1;
    if (y == 6 && x >= 1 && x <= 3) return 1;
    return 0;
}

// 字符'K'的5x7点阵
int getCharK(int x, int y) {
    if (y == 0 && (x == 0 || x == 4)) return 1;
    if (y == 1 && (x == 0 || x == 3)) return 1;
    if (y == 2 && (x == 0 || x == 2)) return 1;
    if (y == 3 && x == 1) return 1;
    if (y == 4 && (x == 0 || x == 2)) return 1;
    if (y == 5 && (x == 0 || x == 3)) return 1;
    if (y == 6 && (x == 0 || x == 4)) return 1;
    return 0;
}

// 字符'E'的5x7点阵
int getCharE(int x, int y) {
    if (y == 0 && x >= 0 && x <= 4) return 1;
    if (y == 1 && x == 0) return 1;
    if (y == 2 && x == 0) return 1;
    if (y == 3 && x <= 3) return 1;
    if (y == 4 && x == 0) return 1;
    if (y == 5 && x == 0) return 1;
    if (y == 6 && x >= 0 && x <= 4) return 1;
    return 0;
}

// 字符'D'的5x7点阵
int getCharD(int x, int y) {
    if (y == 0 && x <= 3) return 1;
    if (y == 1 && (x == 0 || x == 4)) return 1;
    if (y == 2 && (x == 0 || x == 4)) return 1;
    if (y == 3 && (x == 0 || x == 4)) return 1;
    if (y == 4 && (x == 0 || x == 4)) return 1;
    if (y == 5 && (x == 0 || x == 4)) return 1;
    if (y == 6 && x <= 3) return 1;
    return 0;
}

// 字符'!'的5x7点阵
int getCharExcl(int x, int y) {
    if (y == 0 && x == 2) return 1;
    if (y == 1 && x == 2) return 1;
    if (y == 2 && x == 2) return 1;
    if (y == 3 && x == 2) return 1;
    if (y == 4 && x == 2) return 1;
    if (y == 5) return 0;
    if (y == 6 && x == 2) return 1;
    return 0;
}

// 获取字符（简化版，只支持部分字符）
int getChar(int charIndex, int x, int y) {
    // "ORB PICKED!" = O, R, B, space, P, I, C, K, E, D, !
    if (charIndex == 0) return getCharO(x, y);      // O
    if (charIndex == 1) return getCharR(x, y);       // R
    if (charIndex == 2) return getCharB(x, y);       // B
    if (charIndex == 3) return 0;                    // space
    if (charIndex == 4) return getCharP(x, y);       // P
    if (charIndex == 5) return getCharI(x, y);       // I
    if (charIndex == 6) return getCharC(x, y);       // C
    if (charIndex == 7) return getCharK(x, y);       // K
    if (charIndex == 8) return getCharE(x, y);       // E
    if (charIndex == 9) return getCharD(x, y);       // D
    if (charIndex == 10) return getCharExcl(x, y);   // !
    return 0;
}

void main()
{
    // 计算相对于quad中心的位置（标准化到[-1, 1]）
    vec2 center = screenSize * 0.5;
    vec2 offset = (fragPos - center) / 300.0; // 300是quad的一半宽度（放大后）
    
    // 绘制带边框的提示框
    float border = 0.02;
    if (abs(offset.x) > 0.98 || abs(offset.y) > 0.98) {
        FragColor = vec4(textColor, alpha);
        return;
    }
    
    // 背景（半透明黑色）
    vec3 bgColor = vec3(0.0, 0.0, 0.0);
    
    // 计算文字区域（在quad中心，稍微偏上）
    // 文字区域：宽约0.9，高约0.35，位置在中心偏上（放大字体）
    vec2 textAreaSize = vec2(1.3, 0.35);
    vec2 textAreaPos = vec2(0.0, 0.0); // 居中
    vec2 textOffset = offset - textAreaPos;
    
    // 检查是否在文字区域内
    if (abs(textOffset.x) < textAreaSize.x * 0.5 && abs(textOffset.y) < textAreaSize.y * 0.5) {
        // 计算字符索引和像素位置
        // 总共11个字符（包括空格和感叹号）
        int numChars = 11;
        float charWidth = textAreaSize.x / float(numChars);
        
        // 将textOffset.x从[-textAreaSize.x*0.5, textAreaSize.x*0.5]映射到[0, numChars*charWidth]
        float normalizedX = (textOffset.x + textAreaSize.x * 0.5);
        int charIndex = int(normalizedX / charWidth);
        
        // 限制charIndex范围
        if (charIndex >= 0 && charIndex < numChars) {
            // 计算字符内的像素位置（5x7点阵）
            float charLocalX = mod(normalizedX, charWidth) / charWidth; // [0, 1]
            float charLocalY = (textOffset.y + textAreaSize.y * 0.5) / textAreaSize.y; // [0, 1]
            
            // 映射到5x7点阵坐标
            int pixelX = int(charLocalX * 5.0);
            int pixelY = int(charLocalY * 7.0);
            
            // 限制像素坐标范围
            if (pixelX >= 0 && pixelX < 5 && pixelY >= 0 && pixelY < 7) {
                // 获取字符像素
                int pixel = getChar(charIndex, pixelX, pixelY);
                
                if (pixel == 1) {
                    // 绘制文字像素
                    FragColor = vec4(textColor, alpha);
                    return;
                }
            }
        }
    }
    
    // 背景（半透明黑色）
    FragColor = vec4(bgColor, alpha * 0.8);
}
