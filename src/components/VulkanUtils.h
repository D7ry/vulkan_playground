// Utility functions for Vulkan boilterplate functions
#pragma once
#include <vulkan/vulkan_core.h>
namespace VulkanUtils {

VkCommandBuffer beginSingleTimeCommands(VkDevice device, VkCommandPool commandPool);
void endSingleTimeCommands(VkCommandBuffer commandBuffer, VkDevice device, VkQueue graphicsQueue, VkCommandPool commandPool);

void createCommandPool(VkCommandPool* commandPool, VkCommandPoolCreateFlags flags, uint32_t queueFamilyIndex, VkDevice device);

void createCommandBuffers(VkCommandBuffer* commandBuffer, uint32_t commandBufferCount, VkCommandPool commandPool, VkDevice device);

void createCommandBuffers(std::vector<VkCommandBuffer>& commandBufferes, uint32_t commandBufferCount, VkCommandPool commandPool, VkDevice device);

void createCommandPoolAndBuffers(
        VkCommandPool& commandPool,
        VkCommandBuffer& commandBuffer,
        uint32_t commandBufferCount,
        VkCommandPoolCreateFlags flags,
        uint32_t queueFamilyIndex,
        VkDevice device
);

/**
 * @brief Look for the memory flag that suits TYPEFILTER purposes.
 *
 * @param physicalDevice
 * @param typeFilter
 * @param properties
 * @return uint32_t
 */
uint32_t findMemoryType(VkPhysicalDevice physicalDevice, uint32_t typeFilter, VkMemoryPropertyFlags properties);

void createBuffer(
        VkPhysicalDevice physicalDevice,
        VkDevice device,
        VkDeviceSize size,
        VkBufferUsageFlags usage,
        VkMemoryPropertyFlags properties,
        VkBuffer& buffer,
        VkDeviceMemory& bufferMemory
);

void vkMemCopy(void* src, VkDeviceMemory dstMemory, VkDeviceSize size, VkDevice dstDevice);

void copyBuffer(VkDevice device, VkCommandPool commandPool, VkQueue queue, VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);

VkImageView createImageView(VkImage& textureImage, VkDevice& logicalDevice);

} // namespace VulkanUtils