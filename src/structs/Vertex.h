#pragma once
#include <glm/glm.hpp>
#include <vulkan/vulkan_core.h>
struct Vertex {
        /**
         * @brief Vertex struct; note all vertex shaders that uses this struct must have the same layout
         * 
         */
        // 8 bytes
        glm::vec3 pos;
        // 12 bytes 
        glm::vec3 color;
        glm::vec2 texCoord;

        static VkVertexInputBindingDescription GetBindingDescription();

        static std::unique_ptr<std::vector<VkVertexInputAttributeDescription>> GetAttributeDescriptions();
};


/** Format cheatsheet

float: VK_FORMAT_R32_SFLOAT
vec2: VK_FORMAT_R32G32_SFLOAT
vec3: VK_FORMAT_R32G32B32_SFLOAT
vec4: VK_FORMAT_R32G32B32A32_SFLOAT

*/