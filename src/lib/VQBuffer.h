#pragma once
#include <vulkan/vulkan.h>
class VQBuffer {
        VkDevice device;
        VkBuffer buffer;
        VkDeviceMemory bufferMemory;
        VkDeviceSize size;
        void* mappedAddress;
        void cleanup() {
                if (buffer) {
                        vkDestroyBuffer(device, buffer, nullptr);
                }
                if (bufferMemory) {
                        vkFreeMemory(device, bufferMemory, nullptr);
                }
                mappedAddress = nullptr;
        }
};
