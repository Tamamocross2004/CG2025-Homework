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
uniform vec2 liftedTileRegionMin;      // 翘起区域最小边界 (x, z)
uniform vec2 liftedTileRegionMax;      // 翘起区域最大边界 (x, z)

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
            // 地板使用非常宽松的衰减参数，扩大光照范围
            att = 1.0 / (1.0 + 0.1 * d + 0.05 * d * d);
        } else {
            // 其他对象使用原来的衰减
            att = 1.0 / (1.0 + 0.35 * d + 0.44 * d * d);
        }
        
        float diffL = max(dot(norm, L), 0.0);
        vec3 diffuseL = lampLight.color * diffL * baseColor;
        
        vec3 R = reflect(-L, norm);
        float specL = pow(max(dot(viewDir, R), 0.0), 16.0);
        vec3 specularL = lampLight.color * specL * baseColor * 0.5;
        
        // 根据是否是地板来调整强度倍数
        // 地板使用较大倍数，让地板更亮；墙壁和天花板使用较小倍数
        float intensityMultiplier;
        if (isFloor) {
            // 地板使用较大倍数，让地板更亮
            intensityMultiplier = 2.5;
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