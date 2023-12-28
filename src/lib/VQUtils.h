#pragma once
#include "lib/VQDevice.h"
#include <vulkan/vulkan.h>


#include "VQBuffer.h"
#include "structs/Vertex.h"

namespace VQUtils {
uint32_t findMemoryType(VkPhysicalDevice physicalDevice, uint32_t typeFilter, VkMemoryPropertyFlags properties);

void createIndexBuffer(const std::vector<uint32_t>& indices, VQBuffer& buffer, VQDevice& vqDevice);

void createVertexBuffer(const std::vector<Vertex>& vertices, VQBuffer& buffer, VQDevice& vqDevice);

/**
 * @brief Loads from the mesh file a model's data into a vertex buffer and index buffer.
 * Caller is responsible for freeing the buffer's resources.
 */
void meshToBuffer(const char* meshFilePath, VQDevice& vqDevice, VQBuffer& vertexBuffer, VQBuffer& indexBuffer);
} // namespace VQUtils
