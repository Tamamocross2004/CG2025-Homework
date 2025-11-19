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