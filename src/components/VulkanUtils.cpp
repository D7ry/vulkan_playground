#include "VulkanUtils.h"
#include <vulkan/vulkan_core.h>
VkCommandBuffer VulkanUtils::beginSingleTimeCommands(VkDevice device, VkCommandPool commandPool) {
        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandPool = commandPool;
        allocInfo.commandBufferCount = 1;

        VkCommandBuffer commandBuffer;
        vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer);

        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

        vkBeginCommandBuffer(commandBuffer, &beginInfo);

        return commandBuffer;
}
void VulkanUtils::endSingleTimeCommands(VkCommandBuffer commandBuffer, VkDevice device, VkQueue graphicsQueue, VkCommandPool commandPool) {
        vkEndCommandBuffer(commandBuffer);

        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffer;

        vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
        vkQueueWaitIdle(graphicsQueue);

        vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
}
void VulkanUtils::createCommandBuffers(VkCommandBuffer* commandBuffer, uint32_t commandBufferCount, VkCommandPool commandPool, VkDevice device) {
        VkCommandBufferAllocateInfo commandBufferAllocateInfo = {};
        commandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        commandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        commandBufferAllocateInfo.commandPool = commandPool;
        commandBufferAllocateInfo.commandBufferCount = commandBufferCount;
        if (vkAllocateCommandBuffers(device, &commandBufferAllocateInfo, commandBuffer) != VK_SUCCESS) {
                FATAL("Could not allocate command buffers");
        }
}
void VulkanUtils::createCommandBuffers(
        std::vector<VkCommandBuffer>& commandBuffers,
        uint32_t commandBufferCount,
        VkCommandPool commandPool,
        VkDevice device
) {
        commandBuffers.resize(commandBufferCount);
        createCommandBuffers(commandBuffers.data(), commandBufferCount, commandPool, device);
}
void VulkanUtils::createCommandPool(VkCommandPool* commandPool, VkCommandPoolCreateFlags flags, uint32_t queueFamilyIndex, VkDevice device) {
        VkCommandPoolCreateInfo commandPoolCreateInfo = {};
        commandPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        commandPoolCreateInfo.queueFamilyIndex = queueFamilyIndex;
        commandPoolCreateInfo.flags = flags;

        if (vkCreateCommandPool(device, &commandPoolCreateInfo, nullptr, commandPool) != VK_SUCCESS) {
                FATAL("Could not create graphics command pool");
        }
}
void VulkanUtils::createCommandPoolAndBuffers(
        VkCommandPool& commandPool,
        VkCommandBuffer& commandBuffer,
        uint32_t commandBufferCount,
        VkCommandPoolCreateFlags flags,
        uint32_t queueFamilyIndex,
        VkDevice device
) {
        createCommandPool(&commandPool, flags, queueFamilyIndex, device);
        createCommandBuffers(&commandBuffer, commandBufferCount, commandPool, device);
}

uint32_t VulkanUtils::findMemoryType(VkPhysicalDevice physicalDevice, uint32_t typeFilter, VkMemoryPropertyFlags properties) {
        VkPhysicalDeviceMemoryProperties memProperties;
        vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

        for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
                if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
                        return i;
                }
        }

        FATAL("Failed to find suitable memory type!");
}
void VulkanUtils::createBuffer(
        VkPhysicalDevice physicalDevice,
        VkDevice device,
        VkDeviceSize size,
        VkBufferUsageFlags usage,
        VkMemoryPropertyFlags properties,
        VkBuffer& buffer,
        VkDeviceMemory& bufferMemory
) {
        VkBufferCreateInfo bufferInfo{};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.size = size;
        bufferInfo.usage = usage;
        bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        if (vkCreateBuffer(device, &bufferInfo, nullptr, &buffer) != VK_SUCCESS) {
                FATAL("Failed to create VK buffer!");
        }

        VkMemoryRequirements memRequirements;
        vkGetBufferMemoryRequirements(device, buffer, &memRequirements);

        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = findMemoryType(physicalDevice, memRequirements.memoryTypeBits, properties);

        if (vkAllocateMemory(device, &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS) {
                FATAL("Failed to allocate device memory for buffer creation!");
        }

        vkBindBufferMemory(device, buffer, bufferMemory, 0);
}
void VulkanUtils::vkMemCopy(void* src, VkDeviceMemory dstMemory, VkDeviceSize size, VkDevice dstDevice) {
        void* data;
        vkMapMemory(dstDevice, dstMemory, 0, size, 0, &data);
        memcpy(data, src, static_cast<size_t>(size));
        vkUnmapMemory(dstDevice, dstMemory);
}
void VulkanUtils::copyBuffer(VkDevice device, VkCommandPool commandPool, VkQueue queue, VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size) {
        VkCommandBuffer commandBuffer = beginSingleTimeCommands(device, commandPool);

        VkBufferCopy copyRegion{};
        copyRegion.size = size;
        vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

        endSingleTimeCommands(commandBuffer, device, queue, commandPool);
}
