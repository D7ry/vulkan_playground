#version 450

// global UBO
layout(binding = 0) uniform UBOStatic {
    mat4 view;
    mat4 proj;
    float timeSinceStartSeconds; // time in seconds since engine start
    float sinWave;               // a number interpolating between [0,1]
    bool flip;                   // a switch that gets flipped every frame
} uboStatic;

struct InstanceData
{
    mat4 model;
    float transparency;
    int textureAlbedo;
    int drawCmdId; // not used
};

// alignment: 16 byte
layout(std140, binding = 1) readonly buffer InstanceDataArray {
    InstanceData data[];
} instanceDataArray;

// use 430 layout for 4-byte packed int array
layout(std430, binding = 2) readonly buffer InstanceIndexArray {
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

layout(location = 7) flat out int flip;

layout(location=6) flat out int glInstanceIdx;

vec3 globalLightPos = vec3(-6, -3, 0.0);


#extension GL_EXT_debug_printf : enable

void main() {
    flip = uboStatic.flip == true ? 1 : 0;
    // instance index has been tested to have no problem.
    // gl_InstanceIndex: cow has 0, viking room has 10
    // something is wrong with ssbo indexing
    // indices[0] and indices[10] both == 0 -- means they're not written into
    // -- or the way i read SSBO is wrong
    glInstanceIdx = gl_InstanceIndex;
    int instanceIndex = instanceIndexArray.indices[gl_InstanceIndex];
    //instanceIndex = 1;
    //glInstanceIdx = instanceIndexArray.indices[20]; // should be 1, got 0 from frag printing

    mat4 model = instanceDataArray.data[instanceIndex].model;

    gl_Position = uboStatic.proj * uboStatic.view * model * vec4(inPosition, 1.0);
    fragColor = inColor;
    fragTexCoord = inTexCoord;

    mat3 normalMatrix = transpose(inverse(mat3(model))); // Calculate the normal matrix
    fragNormal = normalize(normalMatrix * inNormal); // Transform the normal and pass it to the fragment shader

    fragPos = vec3(model * vec4(inPosition, 1.0)); // Transform the vertex position to world space
    fragGlobalLightPos = globalLightPos; // Pass the light position in world space to the fragment shader

    fragTexIndex = instanceDataArray.data[instanceIndex].textureAlbedo;
}
