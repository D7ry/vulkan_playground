#pragma once
#include <vulkan/vulkan_core.h>

struct MetaBuffer {
        VkDevice device;
        VkBuffer buffer;
        VkDeviceMemory bufferMemory;
        VkDeviceSize size;
        void* bufferAddress;
        void cleanup() {
                if (buffer) {
                        vkDestroyBuffer(device, buffer, nullptr);
                }
                if (bufferMemory) {
                        vkFreeMemory(device, bufferMemory, nullptr);
                }
                bufferAddress = nullptr;
        }
};

MetaBuffer MetaBuffer_Create(
        VkDevice device,
        VkPhysicalDevice physicalDevice,
        VkDeviceSize size,
        VkBufferUsageFlags usage,
        VkMemoryPropertyFlags properties
);