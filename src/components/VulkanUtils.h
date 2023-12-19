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

} // namespace VulkanUtils