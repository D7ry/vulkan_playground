#pragma once
#include "TextureManager.h"
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>
#include "MetaBuffer.h"

/**
 * @brief A graphics pipeline with its own descriptor set layout, descriptor sets, descriptor pool, pipeline layout,
 * shaders. This is extremely under-optimized.
 *
 */
struct MetaPipeline {
        VkPipeline pipeline;
        VkPipelineLayout pipelineLayout;

        VkDescriptorSetLayout descriptorSetLayout;
        VkDescriptorPool descriptorPool;
        std::vector<VkDescriptorSet> descriptorSets;

        inline void Cleanup(VkDevice device) {
                INFO("Cleaning up meta pipeline...");
                vkDestroyPipeline(device, pipeline, nullptr);
                vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
                vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);
                vkDestroyDescriptorPool(device, descriptorPool, nullptr);
        }
};



struct GenerticMetaPipelineUniformBuffer {
        MetaBuffer staticUniformBuffer;
        MetaBuffer dynamicUniformBuffer;
};

MetaPipeline CreateGenericMetaPipeline(
        VkDevice logicalDevice,
        uint32_t numDescriptorSets,
        std::string texturePath,
        std::string vertShaderPath,
        std::string fragShaderPath,
        VkExtent2D swapchainExtent,
        const std::unique_ptr<TextureManager>& textureManager,
        std::vector<GenerticMetaPipelineUniformBuffer>& uniformBuffers,
        uint32_t numDynamicUniformBuffer,
        uint32_t staticUboRange,
        uint32_t dynamicUboRange,
        VkRenderPass renderPass
);