#include "VulkanUtils.h"
#include <vulkan/vulkan_core.h>
#include "MetaBuffer.h"
MetaBuffer MetaBuffer_Create(
        VkDevice device,
        VkPhysicalDevice physicalDevice,
        VkDeviceSize size,
        VkBufferUsageFlags usage,
        VkMemoryPropertyFlags properties
) {
        MetaBuffer metaBuffer{};

        metaBuffer.device = device;
        metaBuffer.size = size;

        VkBufferCreateInfo bufferInfo{};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.size = size;
        bufferInfo.usage = usage;
        bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        if (vkCreateBuffer(device, &bufferInfo, nullptr, &metaBuffer.buffer) != VK_SUCCESS) {
                FATAL("Failed to create VK buffer!");
        }

        VkMemoryRequirements memRequirements;
        vkGetBufferMemoryRequirements(device, metaBuffer.buffer, &memRequirements);

        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex
                = VulkanUtils::findMemoryType(physicalDevice, memRequirements.memoryTypeBits, properties);

        if (vkAllocateMemory(device, &allocInfo, nullptr, &metaBuffer.bufferMemory) != VK_SUCCESS) {
                FATAL("Failed to allocate device memory for buffer creation!");
        }

        vkBindBufferMemory(device, metaBuffer.buffer, metaBuffer.bufferMemory, 0);

        vkMapMemory(device, metaBuffer.bufferMemory, 0, metaBuffer.size, 0, &metaBuffer.bufferAddress);

        return metaBuffer;
}