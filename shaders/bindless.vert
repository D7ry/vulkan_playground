#version 450

// global UBO
layout(binding = 0) uniform UBOStatic {
    mat4 view;
    mat4 proj;
} uboStatic;

struct InstanceData
{
    mat4 model;
    float transparency;
    int textureId;
    int drawCmdId; // not used
};

layout(std140, binding = 1) readonly buffer InstanceDataArray {
    InstanceData data[];
} instanceDataArray;

layout(std140, binding = 2) readonly buffer InstanceIndexArray {
    int indices[];
} instanceIndexArray;


layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec2 inTexCoord;
layout(location = 3) in vec3 inNormal;

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec2 fragTexCoord;
layout(location = 2) out vec3 fragNormal;
layout(location = 3) out vec3 fragPos; // frag position in view space
layout(location = 4) out vec3 fragGlobalLightPos; // light position in view space
layout(location = 5) out int fragTexIndex; // texture index

layout(location=6) flat out int glInstanceIdx;

vec3 globalLightPos = vec3(-6, -3, 0.0);

void main() {
    int instanceIndex = instanceIndexArray.indices[gl_InstanceIndex];

    glInstanceIdx = instanceIndex;
    mat4 model = instanceDataArray.data[instanceIndex].model;

    gl_Position = uboStatic.proj * uboStatic.view * model * vec4(inPosition, 1.0);
    fragColor = inColor;
    fragTexCoord = inTexCoord;

    mat3 normalMatrix = transpose(inverse(mat3(model))); // Calculate the normal matrix
    fragNormal = normalize(normalMatrix * inNormal); // Transform the normal and pass it to the fragment shader

    fragPos = vec3(model * vec4(inPosition, 1.0)); // Transform the vertex position to world space
    fragGlobalLightPos = globalLightPos; // Pass the light position in world space to the fragment shader

    fragTexIndex = instanceDataArray.data[instanceIndex].textureId;
}
