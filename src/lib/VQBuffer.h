#pragma once
#include <structs/Vertex.h>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

struct VQBuffer
{
        VkDevice device = VK_NULL_HANDLE;
        VkBuffer buffer = VK_NULL_HANDLE;
        VkDeviceMemory bufferMemory = VK_NULL_HANDLE;
        VkDeviceSize size = 0;
        void* bufferAddress = nullptr;

        /**
         * @brief Clean up all the resources held by this buffer.
         */
        void Cleanup()
        {
                if (buffer == VK_NULL_HANDLE && bufferMemory == VK_NULL_HANDLE) {
                        return;
                }
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

struct VQBufferIndex : VQBuffer
{
        uint32_t numIndices; // how many indices are there?
        uint32_t indexSize;  // how large is one index?
};

struct VQBufferVertex
{
        VQBuffer vqBuffer;
        std::vector<Vertex> vertices;
};
