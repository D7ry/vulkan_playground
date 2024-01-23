#pragma once
#include "components/TextureManager.h"
#include "lib/VQBuffer.h"
#include "lib/VQDevice.h"
#include <memory>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

#include "RenderGroup.h"

/**
 * @brief A graphics pipeline with its own descriptor set layout, descriptor sets, descriptor pool, pipeline layout,
 * shaders. This is extremely under-optimized.
 *
 */
struct MetaPipeline
{
        VkPipeline pipeline;
        VkPipelineLayout pipelineLayout;

        VkDescriptorSetLayout descriptorSetLayout;
        VkDescriptorPool descriptorPool;

        VkDevice device;

        inline void Cleanup()
        {
                INFO("Cleaning up meta pipeline...");
                vkDestroyPipeline(device, pipeline, nullptr);
                vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
                vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);
                vkDestroyDescriptorPool(device, descriptorPool, nullptr);
        }

        inline void AllocateDescriptorSets(RenderGroup& renderGroup)
        {

                std::vector<VkDescriptorSetLayout> layouts(NUM_INTERMEDIATE_FRAMES, this->descriptorSetLayout);
                VkDescriptorSetAllocateInfo allocInfo{};
                allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
                allocInfo.descriptorPool = this->descriptorPool;
                allocInfo.descriptorSetCount = NUM_INTERMEDIATE_FRAMES;
                allocInfo.pSetLayouts = layouts.data();

                renderGroup.descriptorSets.resize(NUM_INTERMEDIATE_FRAMES);

                if (vkAllocateDescriptorSets(this->device, &allocInfo, renderGroup.descriptorSets.data())
                    != VK_SUCCESS) {
                        FATAL("Failed to allocate descriptor sets!");
                }

                renderGroup.descriptorPool = this->descriptorPool;
        }
};

MetaPipeline CreateGenericMetaPipeline(
        std::shared_ptr<VQDevice> device,
        std::vector<RenderGroup>& renderGroups,
        std::string vertexShader,
        std::string fragmentShader,
        VkRenderPass renderPass
);