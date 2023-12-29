#include "Vertex.h"
#include <vulkan/vulkan_core.h>

VkFormat FormatVec3 = VK_FORMAT_R32G32B32_SFLOAT;
VkFormat FormatVec2 = VK_FORMAT_R32G32_SFLOAT;

VkVertexInputBindingDescription Vertex::GetBindingDescription() {
        // specifies number of bytes between data entries, and whether to move to the next data entry after each vertex or instance
        VkVertexInputBindingDescription bindingDescription{};
        bindingDescription.binding = 0;
        bindingDescription.stride = sizeof(Vertex);                 // size of a single vertex object
        bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX; // set to VK_VERTEX_INPUT_RATE_INSTANCE for instanced rendering(later)

        return bindingDescription;
}
std::unique_ptr<std::vector<VkVertexInputAttributeDescription>> Vertex::GetAttributeDescriptions() {
        // specifies how to extract a vertex attribute from a chunk of vertex data originating from a binding description
        std::unique_ptr<std::vector<VkVertexInputAttributeDescription>> attributeDescriptions = std::make_unique<std::vector<VkVertexInputAttributeDescription>>(4);

        // pos
        attributeDescriptions->at(0).binding = 0;
        attributeDescriptions->at(0).location = 0;                     // location directive of the input in the vertex shader
        attributeDescriptions->at(0).format = FormatVec3; // vec3
        attributeDescriptions->at(0).offset = offsetof(Vertex, pos);   // byte offset of the data from the start of the per-vertex data

        // color
        attributeDescriptions->at(1).binding = 0;
        attributeDescriptions->at(1).location = 1;
        attributeDescriptions->at(1).format = FormatVec3; // vec3
        attributeDescriptions->at(1).offset = offsetof(Vertex, color);

        // vertex
        attributeDescriptions->at(2).binding = 0;
        attributeDescriptions->at(2).location = 2;
        attributeDescriptions->at(2).format = FormatVec2; // vec2
        attributeDescriptions->at(2).offset = offsetof(Vertex, texCoord);

        // normal
        attributeDescriptions->at(3).binding = 0;
        attributeDescriptions->at(3).location = 3;
        attributeDescriptions->at(3).format = FormatVec3; // vec3
        attributeDescriptions->at(3).offset = offsetof(Vertex, normal);

        return std::move(attributeDescriptions);
}
