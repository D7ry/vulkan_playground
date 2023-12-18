#version 450
layout(location = 0) in vec2 inPosition;
layout(location = 1) in vec3 inColor;
layout(location = 0) out vec3 fragColor;

layout(binding =  0) uniform UniformBufferObject {
    mat4 model; // model matrix
    mat4 view; // camera
    mat4 proj; // projection
} ubo;

void main() {
    gl_Position = ubo.proj * ubo.view * ubo.model * vec4(inPosition, 0.0, 1.0);
    fragColor = inColor;
}
