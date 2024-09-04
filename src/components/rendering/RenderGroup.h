#pragma once

#include "components/TextureManager.h"
#include "constants.h"
#include "lib/DeletionStack.h"
#include "lib/VQBuffer.h"
#include "lib/VQDevice.h"
#include "lib/VQUtils.h"
class MeshInstance;

/**
 * @brief A render group is a set of meshes that shares the same texture, render
 * pipeline, and therefore shaders. Each mesh has its own dynamic UBO for
 * information such as transform. There are exactly NUMFRAMEINFLIGHT static UBO
 * and dynamic UBO. Render groups that share the render pipeline must share the
 * same buffer layout.
 */
struct RenderGroup
{
    RenderGroup() = delete;

    RenderGroup(
        size_t staticUboSize,
        size_t dynamicUboSize,
        size_t initialDynamicUboCount,
        const std::string& texturePath,
        const std::string& meshPath,
        std::shared_ptr<VQDevice> device
    ) {
        this->texturePath = texturePath;
        this->meshPath = meshPath;
        this->staticUboSize = staticUboSize;
        this->dynamicUboSize = dynamicUboSize;
        this->device = device;
        this->dynamicUboCount = initialDynamicUboCount;
    }

    void InitUniformbuffers() {
        resizeDynamicUbo(this->dynamicUboCount);

        // allocate static UBO
        for (int i = 0; i < NUM_FRAME_IN_FLIGHT; i++) {
            // allocate static ubo
            device->CreateBufferInPlace(
                staticUboSize,
                VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
                    | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                this->uniformBuffers[i].staticUBO
            );
        }

        // update all descriptor sets.
        for (size_t i = 0; i < NUM_FRAME_IN_FLIGHT; i++) {
            VkDescriptorBufferInfo descriptorBufferInfo_static{};
            descriptorBufferInfo_static.buffer
                = this->uniformBuffers[i].staticUBO.buffer;
            descriptorBufferInfo_static.offset = 0;
            descriptorBufferInfo_static.range = this->staticUboSize;

            // TODO: make a more fool-proof abstraction
            VkDescriptorImageInfo descriptorImageInfo{};
            TextureManager::GetSingleton()->GetDescriptorImageInfo(
                this->texturePath, descriptorImageInfo
            );

            std::array<VkWriteDescriptorSet, 2> descriptorWrites{};

            descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrites[0].dstSet = this->descriptorSets[i];
            descriptorWrites[0].dstBinding = 0;
            descriptorWrites[0].dstArrayElement = 0;
            descriptorWrites[0].descriptorType
                = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            descriptorWrites[0].descriptorCount = 1;
            descriptorWrites[0].pBufferInfo = &descriptorBufferInfo_static;

            descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrites[1].dstSet = this->descriptorSets[i];
            descriptorWrites[1].dstBinding = 2;
            descriptorWrites[1].dstArrayElement = 0;
            descriptorWrites[1].descriptorType
                = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            descriptorWrites[1].descriptorCount = 1;
            descriptorWrites[1].pImageInfo = &descriptorImageInfo;

            vkUpdateDescriptorSets(
                device->logicalDevice,
                descriptorWrites.size(),
                descriptorWrites.data(),
                0,
                nullptr
            );
        }
    }

    struct Uniformbuffers
    {
        VQBuffer staticUBO;
        VQBuffer dynamicUBO;
    };

    Uniformbuffers uniformBuffers[NUM_FRAME_IN_FLIGHT];

    std::string texturePath;
    std::string meshPath;

    uint32_t dynamicUboCount;
    std::vector<MeshInstance*> meshRenderers;
    VQBuffer vertexBuffer;
    VQBufferIndex indexBuffer;

    std::vector<VkDescriptorSet> descriptorSets;

    size_t staticUboSize;
    size_t dynamicUboSize;

    std::shared_ptr<VQDevice> device;
    VkDescriptorPool descriptorPool;

    inline void resizeDynamicUbo(size_t dynamicUboCount) {
        for (int i = 0; i < NUM_FRAME_IN_FLIGHT; i++) {
            // reallocate dyamic ubo
            this->uniformBuffers[i].dynamicUBO.Cleanup();
            this->device->CreateBufferInPlace(
                dynamicUboSize * dynamicUboCount,
                VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
                    | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                this->uniformBuffers[i].dynamicUBO
            );
            // point descriptor to newly allocated buffer
            VkDescriptorBufferInfo descriptorBufferInfo_dynamic{};
            descriptorBufferInfo_dynamic.buffer
                = this->uniformBuffers[i].dynamicUBO.buffer;
            descriptorBufferInfo_dynamic.offset = 0;
            descriptorBufferInfo_dynamic.range = this->dynamicUboSize;

            std::array<VkWriteDescriptorSet, 1> descriptorWrites{};
            descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrites[0].dstSet = this->descriptorSets[i];
            descriptorWrites[0].dstBinding = 1;
            descriptorWrites[0].dstArrayElement = 0;
            descriptorWrites[0].descriptorType
                = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
            descriptorWrites[0].descriptorCount = 1;
            descriptorWrites[0].pBufferInfo = &descriptorBufferInfo_dynamic;

            vkUpdateDescriptorSets(
                device->logicalDevice,
                descriptorWrites.size(),
                descriptorWrites.data(),
                0,
                nullptr
            );
        }
    }

    inline void cleanup() {
        vertexBuffer.Cleanup();
        indexBuffer.Cleanup();
        for (Uniformbuffers& frame : this->uniformBuffers) {
            frame.staticUBO.Cleanup();
            frame.dynamicUBO.Cleanup();
        }
        vkFreeDescriptorSets(
            device->logicalDevice,
            descriptorPool,
            descriptorSets.size(),
            descriptorSets.data()
        );
    }

    void addInstance(MeshInstance* renderer) {
        meshRenderers.push_back(renderer);
        INFO("MeshRenderers size: {}", meshRenderers.size());
        if (meshRenderers.size() > dynamicUboCount) {
            dynamicUboCount = std::max(
                meshRenderers.size(),
                static_cast<size_t>(std::ceil(dynamicUboCount * 1.5))
            );
            INFO("Resizing dynamic UBO to {}", dynamicUboCount);
            resizeDynamicUbo(dynamicUboCount
            ); // every time we resize, we add 50% more space
        }
    }

  private:
    DeletionStack deletionStack;
};

template <typename DynamicUBO_T, typename StaticUBO_T>
RenderGroup CreateRenderGroup(
    std::shared_ptr<VQDevice> device,
    const std::string& texturePath,
    const std::string& meshPath,
    size_t initialDynamicUboCount = 1
) {
    VkPhysicalDeviceProperties properties;
    vkGetPhysicalDeviceProperties(device->physicalDevice, &properties);
    size_t minUboAlignment = properties.limits.minUniformBufferOffsetAlignment;

    size_t dynamicAlignment = sizeof(DynamicUBO_T);
    if (minUboAlignment > 0) {
        size_t remainder = dynamicAlignment % minUboAlignment;
        if (remainder > 0) {
            size_t padding = minUboAlignment - remainder;
            dynamicAlignment += padding;
        }
    }

    RenderGroup ret = RenderGroup(
        sizeof(StaticUBO_T),
        dynamicAlignment,
        initialDynamicUboCount,
        texturePath,
        meshPath,
        device
    );
    VQUtils::meshToBuffer(
        meshPath.data(), *device, ret.vertexBuffer, ret.indexBuffer
    );
    return ret;
}
