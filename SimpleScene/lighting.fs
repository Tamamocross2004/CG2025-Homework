#version 330 core
out vec4 FragColor;

in vec3 Normal;  
in vec3 FragPos;  
  
uniform vec3 lightPos; 
uniform vec3 viewPos; 
uniform vec3 lightColor;
uniform vec3 objectColor;

// 台灯点光源
struct PointLight {
    vec3 position;
    vec3 color;
    float intensity;
};

uniform PointLight lampLight;
uniform bool lampOn;

void main()
{
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
        
    vec3 result = (ambient + diffuse + specular) * objectColor;

    // 台灯点光源
    if (lampOn) {
        vec3 L = normalize(lampLight.position - FragPos);
        float d = length(lampLight.position - FragPos);
        
        // 衰减
        float att = 1.0 / (1.0 + 0.35 * d + 0.44 * d * d);
        
        float diffL = max(dot(norm, L), 0.0);
        vec3 diffuseL = lampLight.color * diffL * objectColor;
        
        vec3 R = reflect(-L, norm);
        float specL = pow(max(dot(viewDir, R), 0.0), 16.0);
        vec3 specularL = lampLight.color * specL * objectColor * 0.5;
        
        result += (diffuseL + specularL) * lampLight.intensity * att;
    }
  
    FragColor = vec4(result, 1.0);
} 