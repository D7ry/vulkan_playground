#version 450
layout(binding = 2) uniform sampler2D textureSampler;

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in vec3 fragNormal;
layout(location = 3) in vec3 fragPos;
layout(location = 4) in vec3 fragGlobalLightPos;

layout(location = 0) out vec4 outColor;

vec3 ambientLighting = vec3(0.1, 0.1, 0.1);
vec3 lightSourceColor = vec3(0.7, 1.0, 1.0); // White light
float lightSourceIntensity = 10;

void main() {
    vec4 textureColor = texture(textureSampler, fragTexCoord);
    vec3 diffToLight = fragGlobalLightPos - fragPos;
    vec3 lightDir = normalize(diffToLight);

    float distToLight = length(diffToLight);

    float cosTheta = max(dot(fragNormal, lightDir), 0.0);
    vec3 diffuseLighting = cosTheta * lightSourceColor * lightSourceIntensity / (distToLight * distToLight);

    vec4 diffuseColor = vec4(textureColor.rgb * diffuseLighting, textureColor.a);

    vec4 ambientColor = vec4(textureColor.rgb * ambientLighting.rgb, textureColor.a);


    outColor = diffuseColor + ambientColor;
}