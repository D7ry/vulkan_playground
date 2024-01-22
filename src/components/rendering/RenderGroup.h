#pragma once

#include "lib/VQBuffer.h"
#include "lib/VQDevice.h"

/**
 * @brief A render group is a set of meshes that shares the same texture, render pipeline, and therefore shaders.
 * Each mesh has its own dynamic UBO for information such as transform.
 * There are exactly NUMFRAMEINFLIGHT static UBO and dynamic UBO.
 * Render groups that share the render pipeline must share the same buffer layout.
 */
class MeshRenderer;

struct RenderGroup
{
        RenderGroup() = delete;

        RenderGroup(size_t staticUboSize, size_t dynamicUboSize)
        {
                this->staticUboSize = staticUboSize;
                this->dynamicUboSize = dynamicUboSize;
        }

        std::string texturePath;
        uint32_t dynamicUboCount;
        std::vector<MeshRenderer*> meshRenderers;
        VQBuffer vertexBuffer;
        VQBufferIndex indexBuffer;

        std::vector<VQBuffer> staticUbo;
        std::vector<VQBuffer> dynamicUbo;
        std::vector<VkDescriptorSet> descriptorSets;

        size_t staticUboSize;
        size_t dynamicUboSize;

        VkDevice descriptorSetDevice;
        VkDescriptorPool descriptorPool;

        inline void cleanup()
        {
                vertexBuffer.Cleanup();
                indexBuffer.Cleanup();
                for (VQBuffer& buffer : staticUbo) {
                        buffer.Cleanup();
                }
                for (VQBuffer& buffer : dynamicUbo) {
                        buffer.Cleanup();
                }
                vkFreeDescriptorSets(descriptorSetDevice, descriptorPool, descriptorSets.size(), descriptorSets.data());
        }
};

template <typename DynamicUBO_T, typename StaticUBO_T>
RenderGroup CreateRenderGroup(std::shared_ptr<VQDevice> device)
{
        VkPhysicalDeviceProperties properties;
        vkGetPhysicalDeviceProperties(device->physicalDevice, &properties);
        size_t minUboAlignment = properties.limits.minUniformBufferOffsetAlignment;

        size_t dynamicAlignment = sizeof(DynamicUBO_T);
        if (minUboAlignment > 0) {
                dynamicAlignment = (dynamicAlignment + minUboAlignment - 1) & ~(minUboAlignment - 1);
        }
        return RenderGroup(sizeof(StaticUBO_T), dynamicAlignment);
}
