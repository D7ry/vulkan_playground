#version 450

layout(binding = 0) uniform EngineUboStatic {
    mat4 view;
    mat4 proj;
    float timeSinceStartSeconds;
} engineUboStatic;

layout(binding = 1) uniform UBOStatic {
    vec3 gridColor; // rgb
} uboStatic;


layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec2 inTexCoord;
layout(location = 3) in vec3 inNormal;

layout(location = 0) out vec3 fragColor;

void main() {
    mat4 model = mat4(1.0);
    gl_Position = engineUboStatic.proj * engineUboStatic.view * model * vec4(inPosition, 1.0);

    fragColor = uboStatic.gridColor;
    
    float sinWave = sin(engineUboStatic.timeSinceStartSeconds);
    sinWave = (sinWave + 1) / 2; // [-1,1] -> [0, 1]

    // logitech breathing effect
    fragColor.r = 0.3;
    fragColor.g = mix(0.3, 1, sinWave);
    fragColor.b = mix(0.3, 1, sinWave);
}
