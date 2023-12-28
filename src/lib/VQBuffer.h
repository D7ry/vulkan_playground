#pragma once
#include <vulkan/vulkan.h>
#include <structs/Vertex.h>

struct VQBuffer {
        VkDevice device;
        VkBuffer buffer;
        VkDeviceMemory bufferMemory;
        VkDeviceSize size;
        void* bufferAddress;
        /**
         * @brief Clean up all the resources held by this buffer.
         */
        void Cleanup() 
        {
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