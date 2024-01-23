#pragma once

#include "components/TextureManager.h"
#include "constants.h"
#include "lib/VQBuffer.h"
#include "lib/VQDevice.h"
class MeshRenderer;

/**
 * @brief A render group is a set of meshes that shares the same texture, render pipeline, and therefore shaders.
 * Each mesh has its own dynamic UBO for information such as transform.
 * There are exactly NUMFRAMEINFLIGHT static UBO and dynamic UBO.
 * Render groups that share the render pipeline must share the same buffer layout.
 */
struct RenderGroup
{
        RenderGroup() = delete;

        RenderGroup(
                size_t staticUboSize,
                size_t dynamicUboSize,
                size_t initialDynamicUboCount,
                const std::string& texturePath,
                std::shared_ptr<VQDevice> device
        )
        {
                this->texturePath = texturePath;
                this->staticUboSize = staticUboSize;
                this->dynamicUboSize = dynamicUboSize;
                this->device = device;
                this->staticUbo.resize(NUM_INTERMEDIATE_FRAMES);
                this->dynamicUbo.resize(NUM_INTERMEDIATE_FRAMES);
                this->dynamicUboCount = initialDynamicUboCount;
        }

        void InitUniformbuffers()
        {
                resizeDynamicUbo(this->dynamicUboCount);

                // allocate static UBO
                for (int i = 0; i < NUM_INTERMEDIATE_FRAMES; i++) {
                        // allocate static ubo
                        staticUbo[i] = device->CreateBuffer(
                                staticUboSize,
                                VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
                        );
                }

                // update all descriptor sets.
                for (size_t i = 0; i < NUM_INTERMEDIATE_FRAMES; i++) {
                        VkDescriptorBufferInfo descriptorBufferInfo_static{};
                        descriptorBufferInfo_static.buffer = this->staticUbo[i].buffer;
                        descriptorBufferInfo_static.offset = 0;
                        descriptorBufferInfo_static.range = this->staticUboSize;

                        VkDescriptorBufferInfo descriptorBufferInfo_dynamic{};
                        descriptorBufferInfo_dynamic.buffer = this->dynamicUbo[i].buffer;
                        descriptorBufferInfo_dynamic.offset = 0;
                        descriptorBufferInfo_dynamic.range = this->dynamicUboSize;
                        INFO("Dynamic descriptor range: {}", descriptorBufferInfo_dynamic.range);
                        // TODO: make a more fool-proof abstraction
                        VkDescriptorImageInfo descriptorImageInfo{};
                        TextureManager::GetSingleton()->GetDescriptorImageInfo(this->texturePath, descriptorImageInfo);

                        std::array<VkWriteDescriptorSet, 2> descriptorWrites{};

                        descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                        descriptorWrites[0].dstSet = this->descriptorSets[i];
                        descriptorWrites[0].dstBinding = 0;
                        descriptorWrites[0].dstArrayElement = 0;
                        descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                        descriptorWrites[0].descriptorCount = 1;
                        descriptorWrites[0].pBufferInfo = &descriptorBufferInfo_static;

                        descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                        descriptorWrites[1].dstSet = this->descriptorSets[i];
                        descriptorWrites[1].dstBinding = 2;
                        descriptorWrites[1].dstArrayElement = 0;
                        descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
                        descriptorWrites[1].descriptorCount = 1;
                        descriptorWrites[1].pImageInfo = &descriptorImageInfo;

                        vkUpdateDescriptorSets(
                                device->logicalDevice, descriptorWrites.size(), descriptorWrites.data(), 0, nullptr
                        );
                }
        }

        struct FrameData
        {
                VQBuffer staticUBO;
                VQBuffer dynamicUBO;
                VkDescriptorSet descriptorSet;
        };

        FrameData frameData[NUM_INTERMEDIATE_FRAMES];

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

        std::shared_ptr<VQDevice> device;
        VkDescriptorPool descriptorPool;

        inline void resizeDynamicUbo(size_t dynamicUboCount)
        {
                for (int i = 0; i < NUM_INTERMEDIATE_FRAMES; i++) {
                        dynamicUbo[i].Cleanup();
                        dynamicUbo[i] = device->CreateBuffer(
                                dynamicUboSize * dynamicUboCount,
                                VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
                        );
                }
                this->dynamicUboCount = dynamicUboCount;

                for (size_t i = 0; i < NUM_INTERMEDIATE_FRAMES; i++) {
                        VkDescriptorBufferInfo descriptorBufferInfo_dynamic{};
                        descriptorBufferInfo_dynamic.buffer = this->dynamicUbo[i].buffer;
                        descriptorBufferInfo_dynamic.offset = 0;
                        descriptorBufferInfo_dynamic.range = this->dynamicUboSize;
                        INFO("Dynamic descriptor range: {}", descriptorBufferInfo_dynamic.range);
                        VkDescriptorImageInfo descriptorImageInfo{};
                        TextureManager::GetSingleton()->GetDescriptorImageInfo(this->texturePath, descriptorImageInfo);

                        std::array<VkWriteDescriptorSet, 1> descriptorWrites{};
                        descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                        descriptorWrites[0].dstSet = this->descriptorSets[i]; // FIXME: segfault here
                        descriptorWrites[0].dstBinding = 1;
                        descriptorWrites[0].dstArrayElement = 0;
                        descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
                        descriptorWrites[0].descriptorCount = 1;
                        descriptorWrites[0].pBufferInfo = &descriptorBufferInfo_dynamic;

                        vkUpdateDescriptorSets(
                                device->logicalDevice, descriptorWrites.size(), descriptorWrites.data(), 0, nullptr
                        );
                }
        }

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
                vkFreeDescriptorSets(
                        device->logicalDevice, descriptorPool, descriptorSets.size(), descriptorSets.data()
                );
        }

        inline void addRenderer(MeshRenderer* renderer)
        {
                meshRenderers.push_back(renderer);
                INFO("MeshRenderers size: {}", meshRenderers.size());
                if (meshRenderers.size() > dynamicUboCount) {
                        dynamicUboCount
                                = std::max(meshRenderers.size(), static_cast<size_t>(std::ceil(dynamicUboCount * 1.5)));
                        INFO("Resizing dynamic UBO to {}", dynamicUboCount);
                        resizeDynamicUbo(dynamicUboCount); // every time we resize, we add 50% more space
                }
        }
};

template <typename DynamicUBO_T, typename StaticUBO_T>
RenderGroup CreateRenderGroup(
        std::shared_ptr<VQDevice> device,
        std::string& texturePath,
        size_t initialDynamicUboCount = 1
)
{
        VkPhysicalDeviceProperties properties;
        vkGetPhysicalDeviceProperties(device->physicalDevice, &properties);
        size_t minUboAlignment = properties.limits.minUniformBufferOffsetAlignment;

        size_t dynamicAlignment = sizeof(DynamicUBO_T);
        if (minUboAlignment > 0) {
                dynamicAlignment = (dynamicAlignment + minUboAlignment - 1) & ~(minUboAlignment - 1);
        }
        return RenderGroup(sizeof(StaticUBO_T), dynamicAlignment, initialDynamicUboCount, texturePath, device);
}
