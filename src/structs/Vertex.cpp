#include "Vertex.h"
#include <vulkan/vulkan_core.h>

VkFormat FormatVec3 = VK_FORMAT_R32G32B32_SFLOAT;
VkFormat FormatVec2 = VK_FORMAT_R32G32_SFLOAT;

VkVertexInputBindingDescription Vertex::GetBindingDescription() {
    // specifies number of bytes between data entries, and whether to move to the next data entry after each vertex or
    // instance
    VkVertexInputBindingDescription bindingDescription{};
    bindingDescription.binding = 0;
    bindingDescription.stride = sizeof(Vertex); // size of a single vertex object
    bindingDescription.inputRate
        = VK_VERTEX_INPUT_RATE_VERTEX; // set to VK_VERTEX_INPUT_RATE_INSTANCE for instanced rendering(later)

    return bindingDescription;
}

const std::array<VkVertexInputAttributeDescription, 4>* Vertex::GetAttributeDescriptions() {
    static bool initialized = false;
    // specifies how to extract a vertex attribute from a chunk of vertex data originating from a binding description
    static std::array<VkVertexInputAttributeDescription, 4> attributeDescriptions;
    if (!initialized) {
        // layout(location = 0) in vec3 inPosition;
        attributeDescriptions[0].binding = 0;
        attributeDescriptions[0].location = 0;        // location directive of the input in the vertex shader
        attributeDescriptions[0].format = FormatVec3; // vec3
        attributeDescriptions[0].offset
            = offsetof(Vertex, pos); // byte offset of the data from the start of the per-vertex data

        // layout(location = 1) in vec3 inColor;
        attributeDescriptions[1].binding = 0;
        attributeDescriptions[1].location = 1;
        attributeDescriptions[1].format = FormatVec3; // vec3
        attributeDescriptions[1].offset = offsetof(Vertex, color);

        // layout(location = 1) in vec3 inTexCoord
        attributeDescriptions[2].binding = 0;
        attributeDescriptions[2].location = 2;
        attributeDescriptions[2].format = FormatVec2; // vec2
        attributeDescriptions[2].offset = offsetof(Vertex, texCoord);

        // layout(location = 3) in vec3 inNormal;
        attributeDescriptions[3].binding = 0;
        attributeDescriptions[3].location = 3;
        attributeDescriptions[3].format = FormatVec3; // vec3
        attributeDescriptions[3].offset = offsetof(Vertex, normal);
        initialized = true;
    }


    return std::addressof(attributeDescriptions);
}
