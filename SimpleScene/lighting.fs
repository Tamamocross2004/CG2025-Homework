#version 330 core
out vec4 FragColor;

in vec3 Normal;  
in vec3 FragPos;
in vec2 TexCoords;
  
uniform vec3 lightPos; 
uniform vec3 viewPos; 
uniform vec3 lightColor;
uniform vec3 objectColor;

// 纹理（可选）
uniform sampler2D floorTexture;
uniform bool useTexture;  // 是否使用纹理
uniform bool isFloor;     // 是否是地板

// 台灯点光源
struct PointLight {
    vec3 position;
    vec3 color;
    float intensity;
};

uniform PointLight lampLight;
uniform bool lampOn;

// 灵珠点光源
uniform PointLight orbLight;
uniform bool orbLightOn;

// 翘起区域排除（用于主地板）
uniform bool excludeLiftedTileRegion;  // 是否排除翘起区域（只用于主地板）
uniform bool isLiftedTile;             // 是否是翘起地板块（如果是，则不应该被排除）
uniform bool isHole;                   // 是否是无盖长方体（洞）
uniform vec2 liftedTileRegionMin;      // 翘起区域最小边界 (x, z)
uniform vec2 liftedTileRegionMax;      // 翘起区域最大边界 (x, z)

// 砖墙区域排除（用于右墙）
uniform bool excludeBrickSquareRegion; // 是否排除砖墙区域（只用于右墙）
uniform bool isBrickSquare;            // 是否是砖墙正方形（如果是，则不应该被排除）
uniform vec2 brickSquareRegionMin;     // 砖墙区域最小边界 (y, z)
uniform vec2 brickSquareRegionMax;     // 砖墙区域最大边界 (y, z)

// 地砖区域排除（用于主地板，排除z轴尽头的地砖）
uniform bool excludeBrickTileRegion;   // 是否排除地砖区域（只用于主地板）
uniform vec2 brickTile1RegionMin;      // 地砖1区域最小边界 (x, z)
uniform vec2 brickTile1RegionMax;      // 地砖1区域最大边界 (x, z)
uniform vec2 brickTile2RegionMin;      // 地砖2区域最小边界 (x, z)
uniform vec2 brickTile2RegionMax;      // 地砖2区域最大边界 (x, z)

void main()
{
    // 如果是主地板（不是翘起地板块）且需要排除翘起区域，检查当前片段是否在翘起区域内
    if (isFloor && excludeLiftedTileRegion && !isLiftedTile) {
        vec2 fragXZ = FragPos.xz;
        if (fragXZ.x >= liftedTileRegionMin.x && fragXZ.x <= liftedTileRegionMax.x &&
            fragXZ.y >= liftedTileRegionMin.y && fragXZ.y <= liftedTileRegionMax.y) {
            discard; // 丢弃这个片段，不渲染
        }
    }
    
    // 如果是右墙且需要排除砖墙区域，检查当前片段是否在砖墙区域内
    // 右墙是垂直的，所以检查Y和Z坐标
    if (!isFloor && excludeBrickSquareRegion && !isBrickSquare) {
        // 检查是否在右墙内表面附近（x约等于3.95）
        if (FragPos.x >= 3.90f && FragPos.x <= 4.0f) {
            vec2 fragYZ = FragPos.yz;
            if (fragYZ.x >= brickSquareRegionMin.x && fragYZ.x <= brickSquareRegionMax.x &&
                fragYZ.y >= brickSquareRegionMin.y && fragYZ.y <= brickSquareRegionMax.y) {
                discard; // 丢弃这个片段，不渲染
            }
        }
    }
    
    // 如果是主地板且需要排除地砖区域，检查当前片段是否在地砖区域内
    if (isFloor && excludeBrickTileRegion && !isLiftedTile) {
        vec2 fragXZ = FragPos.xz;
        // 检查是否在地砖1范围内
        if (fragXZ.x >= brickTile1RegionMin.x && fragXZ.x <= brickTile1RegionMax.x &&
            fragXZ.y >= brickTile1RegionMin.y && fragXZ.y <= brickTile1RegionMax.y) {
            discard; // 丢弃这个片段，不渲染
        }
        // 检查是否在地砖2范围内
        if (fragXZ.x >= brickTile2RegionMin.x && fragXZ.x <= brickTile2RegionMax.x &&
            fragXZ.y >= brickTile2RegionMin.y && fragXZ.y <= brickTile2RegionMax.y) {
            discard; // 丢弃这个片段，不渲染
        }
    }
    
    // 获取基础颜色（纹理或对象颜色）
    vec3 baseColor = objectColor;
    if (useTexture) {
        baseColor = texture(floorTexture, TexCoords).rgb;
    }
    
    // 环境光
    float ambientStrength = 0.2;
    vec3 ambient = ambientStrength * lightColor;
  	
    // 漫反射 
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(lightPos - FragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * lightColor;
    
    // 镜面反射
    float specularStrength = 0.2;
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 reflectDir = reflect(-lightDir, norm);  
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 8);
    vec3 specular = specularStrength * spec * lightColor;  
        
    vec3 result = (ambient + diffuse + specular) * baseColor;

    // 台灯点光源
    if (lampOn) {
        vec3 L = normalize(lampLight.position - FragPos);
        float d = length(lampLight.position - FragPos);
        
        // 衰减（对地板使用更宽松的衰减，让光照范围更大）
        float att;
        if (useTexture) {
            // 地板使用非常宽松的衰减参数，扩大光照范围（进一步放宽以照亮无盖长方体内部）
            att = 1.0 / (1.0 + 0.05 * d + 0.02 * d * d); // 从0.1/0.05改为0.05/0.02，衰减更慢
        } else {
            // 其他对象使用原来的衰减
            att = 1.0 / (1.0 + 0.35 * d + 0.44 * d * d);
        }
        
        // 对于无盖长方体内部，使用双向光照（让背向光源的面也能接收到光照）
        float diffL;
        if (isHole) {
            // 无盖长方体内部：使用双向光照，让背向光源的面也能接收到一些光照
            // 使用 abs(dot) 然后缩放，这样背向光源的面也能接收到光照
            float dotNL = dot(norm, L);
            diffL = max(dotNL, 0.0) + max(-dotNL, 0.0) * 0.3; // 正向100%，背向30%
        } else {
            // 其他对象：正常单向光照
            diffL = max(dot(norm, L), 0.0);
        }
        vec3 diffuseL = lampLight.color * diffL * baseColor;
        
        vec3 R = reflect(-L, norm);
        float specL = pow(max(dot(viewDir, R), 0.0), 16.0);
        vec3 specularL = lampLight.color * specL * baseColor * 0.5;
        
        // 根据是否是地板来调整强度倍数
        // 地板使用较大倍数，让地板更亮；墙壁和天花板使用较小倍数
        float intensityMultiplier;
        if (isFloor) {
            // 地板使用较大倍数，让地板更亮（特别增加无盖长方体内部的亮度）
            if (isHole) {
                // 无盖长方体内部使用更大的倍数
                intensityMultiplier = 3.0;
            } else {
                intensityMultiplier = 2.0;
            }
        } else if (useTexture) {
            // 使用纹理但不是地板的对象（墙壁、天花板）使用较小倍数
            intensityMultiplier = 0.4;
        } else {
            // 不使用纹理的对象
            intensityMultiplier = 1.0;
        }
        result += (diffuseL + specularL) * lampLight.intensity * att * intensityMultiplier;
    }

    // 灵珠点光源
    if (orbLightOn) {
        vec3 L = normalize(orbLight.position - FragPos);
        float d = length(orbLight.position - FragPos);
        
        // 衰减（使用更宽松的衰减，让光照范围更大，特别适合照亮无盖长方体内部）
        float att = 1.0 / (1.0 + 0.05 * d + 0.02 * d * d);
        
        float diffL = max(dot(norm, L), 0.0);
        vec3 diffuseL = orbLight.color * diffL * baseColor;
        
        vec3 R = reflect(-L, norm);
        float specL = pow(max(dot(viewDir, R), 0.0), 16.0);
        vec3 specularL = orbLight.color * specL * baseColor * 0.5;
        
        // 根据是否是地板来调整强度倍数
        float intensityMultiplier;
        if (isFloor) {
            // 地板使用较大倍数（增加亮度以照亮无盖长方体内部）
            intensityMultiplier = 5.0;
        } else if (useTexture) {
            // 使用纹理但不是地板的对象（墙壁、天花板）使用较小倍数
            intensityMultiplier = 0.4;
        } else {
            // 不使用纹理的对象
            intensityMultiplier = 1.0;
        }
        result += (diffuseL + specularL) * orbLight.intensity * att * intensityMultiplier;
    }
  
    FragColor = vec4(result, 1.0);
} 