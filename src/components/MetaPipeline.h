#pragma once
#include "TextureManager.h"
#include <memory>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>
#include "MetaBuffer.h"
#include "lib/VQDevice.h"
#include "lib/VQBuffer.h"
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

        inline void Cleanup(VkDevice device) {
                INFO("Cleaning up meta pipeline...");
                vkDestroyPipeline(device, pipeline, nullptr);
                vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
                vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);
                vkDestroyDescriptorPool(device, descriptorPool, nullptr);
        }
};



// struct GenerticMetaPipelineUniformBuffer {
//         MetaBuffer staticUniformBuffer;
//         MetaBuffer dynamicUniformBuffer;
// };

// MetaPipeline CreateGenericMetaPipeline(
//         VkDevice logicalDevice,
//         uint32_t numFrameInFlight,
//         std::string texturePath,
//         std::string vertShaderPath,
//         std::string fragShaderPath,
//         std::vector<GenerticMetaPipelineUniformBuffer>& uniformBuffers,
//         uint32_t numDynamicUniformBuffer,
//         uint32_t staticUboRange,
//         uint32_t dynamicUboRange,
//         VkRenderPass renderPass
// ); 

struct RenderGroup;

MetaPipeline CreateGenericMetaPipeline(
        std::shared_ptr<VQDevice> device,
        uint32_t numFrameInFlight,
        uint32_t staticUboRange,
        uint32_t dynamicUboRange,
        std::vector<RenderGroup>& renderGroups,
        std::string vertexShader,
        std::string fragmentShader,
        VkRenderPass renderPass
);