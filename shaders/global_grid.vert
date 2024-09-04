#version 450

layout(binding = 0) uniform UBOStatic {
    mat4 view;
    mat4 proj;
    vec3 gridColor; // rgb
} uboStatic;


layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec2 inTexCoord;
layout(location = 3) in vec3 inNormal;

layout(location = 0) out vec3 fragColor;

void main() {
    mat4 model = mat4(1.0);
    gl_Position = uboStatic.proj * uboStatic.view * model * vec4(inPosition, 1.0);

    fragColor = uboStatic.gridColor;
}
