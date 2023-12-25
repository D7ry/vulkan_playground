#pragma once
#include "TextureManager.h"
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>
/**
 * @brief A graphics pipeline with its own descriptor set layout, descriptor sets, descriptor pool, pipeline layout,
 * shaders, and uniform buffers. This is extremely under-optimized.
 *
 */
struct MetaPipeline {
        VkPipeline pipeline;
        VkPipelineLayout pipelineLayout;

        VkDescriptorSetLayout descriptorSetLayout;
        VkDescriptorPool descriptorPool;
        std::vector<VkDescriptorSet> descriptorSets;
};

MetaPipeline CreateGenericMetaPipeline(
        VkDevice logicalDevice,
        uint32_t numDescriptorSets,
        std::string texturePath,
        std::string vertShaderPath,
        std::string fragShaderPath,
        VkExtent2D swapchainExtent,
        const std::unique_ptr<TextureManager>& textureManager,
        std::vector<VkBuffer>& uniformBuffers,
        VkRenderPass renderPass
);