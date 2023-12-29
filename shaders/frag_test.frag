#version 450
layout(binding = 2) uniform sampler2D textureSampler;

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in vec3 fragNormal;

layout(location = 0) out vec4 outColor;

vec4 ambientColor = vec4(0.1, 0.1, 0.1, 0.1);
vec3 lightSourcePos = vec3(2, 2, 2);
void main() {
    vec4 textureColor = texture(textureSampler, fragTexCoord);

    outColor = vec4(fragNormal, 0.1) * ambientColor;
}