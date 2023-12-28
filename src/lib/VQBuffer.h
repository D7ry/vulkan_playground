#pragma once
#include <vulkan/vulkan.h>
#include <structs/Vertex.h>
#include <vulkan/vulkan_core.h>

struct VQBuffer {
        VkDevice device = VK_NULL_HANDLE;
        VkBuffer buffer = VK_NULL_HANDLE;
        VkDeviceMemory bufferMemory;
        VkDeviceSize size;
        void* bufferAddress;
        /**
         * @brief Clean up all the resources held by this buffer.
         */
        void Cleanup() 
        {
                if (device == VK_NULL_HANDLE) {
                        FATAL("Buffer device not speficied");
                }
                if (buffer) {
                        vkDestroyBuffer(device, buffer, nullptr);
                }
                if (bufferMemory) {
                        vkFreeMemory(device, bufferMemory, nullptr);
                }
                bufferAddress = nullptr;
        }
};

struct VQBufferVertex {
        VQBuffer vqBuffer;
        std::vector<Vertex> vertices;
};

struct VQBufferIndex {
        VQBuffer vqBuffer;
        std::vector<uint32_t> indices;
};