#include "MetaPipeline.h"
#include "ShaderUtils.h"
#include "VulkanUtils.h"
#include "structs/Vertex.h"
#include "structs/UniformBuffer.h"
#include <vulkan/vulkan_core.h>
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
) {
        INFO("Creating genertic meta pipeline...");
        MetaPipeline m{};

        // descriptor set layout
        {
                // UBO static
                VkDescriptorSetLayoutBinding uboStaticBinding{};
                uboStaticBinding.binding = 0; // binding = 0 in shader
                uboStaticBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                uboStaticBinding.descriptorCount = 1; // number of values in the array

                uboStaticBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT; // only used in vertex shader
                uboStaticBinding.pImmutableSamplers = nullptr;            // Optional

                // UBO Dynamic
                VkDescriptorSetLayoutBinding uboDynamicBinding{};
                uboDynamicBinding.binding = 1; // binding = 0 in shader
                uboDynamicBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
                uboDynamicBinding.descriptorCount = 1; // number of values in the array

                uboDynamicBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT; // only used in vertex shader
                uboDynamicBinding.pImmutableSamplers = nullptr;            // Optional

                // image sampler
                VkDescriptorSetLayoutBinding samplerLayoutBinding{};
                samplerLayoutBinding.binding = 2;
                samplerLayoutBinding.descriptorCount = 1;
                samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
                samplerLayoutBinding.stageFlags
                        = VK_SHADER_STAGE_FRAGMENT_BIT; // only used on fragment shader; (may use for vertex shader for
                                                        // height mapping)
                samplerLayoutBinding.pImmutableSamplers = nullptr;

                std::array<VkDescriptorSetLayoutBinding, 3> bindings = {uboStaticBinding, uboDynamicBinding, samplerLayoutBinding};

                VkDescriptorSetLayoutCreateInfo layoutInfo{};
                layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
                layoutInfo.bindingCount = bindings.size(); // number of bindings
                layoutInfo.pBindings = bindings.data();

                if (vkCreateDescriptorSetLayout(logicalDevice, &layoutInfo, nullptr, &m.descriptorSetLayout)
                    != VK_SUCCESS) {
                        FATAL("Failed to create descriptor set layout!");
                }
        }
        // descriptor pool
        {
                VkDescriptorPoolSize poolSizes[]
                        = { {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, static_cast<uint32_t>(numDescriptorSets)},
                        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, static_cast<uint32_t>(numDescriptorSets)},
                           {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, static_cast<uint32_t>(numDescriptorSets)}};

                VkDescriptorPoolCreateInfo poolInfo{};
                poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
                poolInfo.poolSizeCount = sizeof(poolSizes) / sizeof(VkDescriptorPoolSize); // number of pool sizes
                poolInfo.pPoolSizes = poolSizes;
                poolInfo.maxSets = numDescriptorSets * poolInfo.poolSizeCount;

                if (vkCreateDescriptorPool(logicalDevice, &poolInfo, nullptr, &m.descriptorPool) != VK_SUCCESS) {
                        FATAL("Failed to create descriptor pool!");
                }
        }
        // descriptor set
        {
                std::vector<VkDescriptorSetLayout> layouts(numDescriptorSets, m.descriptorSetLayout);
                VkDescriptorSetAllocateInfo allocInfo{};
                allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
                allocInfo.descriptorPool = m.descriptorPool;
                allocInfo.descriptorSetCount = numDescriptorSets;
                allocInfo.pSetLayouts = layouts.data();

                m.descriptorSets.resize(numDescriptorSets);
                if (vkAllocateDescriptorSets(logicalDevice, &allocInfo, m.descriptorSets.data()) != VK_SUCCESS) {
                        FATAL("Failed to allocate descriptor sets!");
                }

                if (uniformBuffers.size() != numDescriptorSets) {
                        FATAL("Uniform buffers size does not match required number of descriptors!");
                }

                for (size_t i = 0; i < numDescriptorSets; i++) {
                        VkDescriptorBufferInfo descriptorBufferInfo_static{};
                        descriptorBufferInfo_static.buffer = uniformBuffers[i].staticUniformBuffer.buffer;
                        descriptorBufferInfo_static.offset = 0;
                        descriptorBufferInfo_static.range = staticUboRange;

                        VkDescriptorBufferInfo descriptorBufferInfo_dynamic{};
                        descriptorBufferInfo_dynamic.buffer = uniformBuffers[i].dynamicUniformBuffer.buffer;
                        descriptorBufferInfo_dynamic.offset = 0;
                        descriptorBufferInfo_dynamic.range = numDynamicUniformBuffer * dynamicUboRange;
                        //TODO: make a more fool-proof abstraction
                        VkDescriptorImageInfo descriptorImageInfo{};
                        textureManager->GetDescriptorImageInfo(texturePath, descriptorImageInfo);

                        std::array<VkWriteDescriptorSet, 3> descriptorWrites{};

                        descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                        descriptorWrites[0].dstSet = m.descriptorSets[i];
                        descriptorWrites[0].dstBinding = 0;
                        descriptorWrites[0].dstArrayElement = 0;
                        descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                        descriptorWrites[0].descriptorCount = 1;
                        descriptorWrites[0].pBufferInfo = &descriptorBufferInfo_static;

                        descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                        descriptorWrites[1].dstSet = m.descriptorSets[i];
                        descriptorWrites[1].dstBinding = 1;
                        descriptorWrites[1].dstArrayElement = 0;
                        descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
                        descriptorWrites[1].descriptorCount = 1;
                        descriptorWrites[1].pBufferInfo = &descriptorBufferInfo_dynamic;

                        descriptorWrites[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                        descriptorWrites[2].dstSet = m.descriptorSets[i];
                        descriptorWrites[2].dstBinding = 2;
                        descriptorWrites[2].dstArrayElement = 0;
                        descriptorWrites[2].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
                        descriptorWrites[2].descriptorCount = 1;
                        descriptorWrites[2].pImageInfo = &descriptorImageInfo;

                        vkUpdateDescriptorSets(
                                logicalDevice, descriptorWrites.size(), descriptorWrites.data(), 0, nullptr
                        );
                }
        }

        // pipeline
        {
                INFO("setting up shader modules...");
                // programmable stages
                VkShaderModule vertShaderModule
                        = ShaderCreation::createShaderModule(logicalDevice, vertShaderPath.data());
                VkShaderModule fragShaderModule
                        = ShaderCreation::createShaderModule(logicalDevice, fragShaderPath.data());

                VkPipelineShaderStageCreateInfo vertShaderStageInfo = {};
                vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
                vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
                vertShaderStageInfo.module = vertShaderModule;
                vertShaderStageInfo.pName = "main";

                VkPipelineShaderStageCreateInfo fragShaderStageInfo = {};
                fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
                fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
                fragShaderStageInfo.module = fragShaderModule;
                fragShaderStageInfo.pName = "main";

                // static stages(vertex and frag shader)
                VkPipelineShaderStageCreateInfo shaderStages[]
                        = {vertShaderStageInfo, fragShaderStageInfo}; // put the 2 stages together.

                INFO("setting up vertex input bindings...");
                VkPipelineVertexInputStateCreateInfo vertexInputInfo = {}; // describes the format of the vertex data.
                vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
                // set up vertex descriptions
                VkVertexInputBindingDescription bindingDescription = Vertex::GetBindingDescription();
                std::unique_ptr<std::vector<VkVertexInputAttributeDescription>> attributeDescriptions
                        = Vertex::GetAttributeDescriptions();

                vertexInputInfo.vertexBindingDescriptionCount = 1;
                vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
                vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions->size());
                vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions->data();

                // Input assembly
                VkPipelineInputAssemblyStateCreateInfo inputAssembly
                        = {}; // describes what kind of geometry will be drawn from the vertices and if primitive
                              // restart should be enabled.
                inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
                inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST; // draw triangles
                inputAssembly.primitiveRestartEnable = VK_FALSE;              // don't restart primitives

                // depth and stencil state
                VkPipelineDepthStencilStateCreateInfo depthStencilCreateInfo{};
                depthStencilCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
                depthStencilCreateInfo.depthTestEnable = VK_TRUE;
                depthStencilCreateInfo.depthWriteEnable = VK_TRUE;

                depthStencilCreateInfo.depthCompareOp
                        = VK_COMPARE_OP_LESS; // low depth == closer object -> cull far object

                depthStencilCreateInfo.depthBoundsTestEnable = VK_FALSE;
                depthStencilCreateInfo.minDepthBounds = 0.0f; // Optional
                depthStencilCreateInfo.maxDepthBounds = 1.0f; // Optional

                depthStencilCreateInfo.stencilTestEnable = VK_FALSE;
                depthStencilCreateInfo.front = {}; // Optional
                depthStencilCreateInfo.back = {};  // Optional

                // dynamic states
                // specifying the dynamic states here so that they are not defined during the pipeline creation,
                // they can be changed at draw time.
                std::vector<VkDynamicState> dynamicStates = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};

                INFO("setting up viewport and scissors...");
                VkPipelineDynamicStateCreateInfo dynamicStateCreateInfo = {};
                dynamicStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
                dynamicStateCreateInfo.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
                dynamicStateCreateInfo.pDynamicStates = dynamicStates.data();

                // viewport and scissors
                VkViewport viewport
                        = {}; // describes the region of the framebuffer that the output will be rendered to.
                viewport.x = 0.0f;
                viewport.y = 0.0f;
                // configure viewport resolution to match the swapchain
                viewport.width = static_cast<float>(swapchainExtent.width);
                viewport.height = static_cast<float>(swapchainExtent.height);
                // don't know what they are for, but keep them at 0.0f and 1.0f
                viewport.minDepth = 0.0f;
                viewport.maxDepth = 1.0f;

                VkRect2D scissor = {}; // clips pixels outside the region
                scissor.offset = {0, 0};
                scissor.extent
                        = swapchainExtent; // we don't want to clip anything, so we use the swapchain extent

                VkPipelineViewportStateCreateInfo viewportState{};
                viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
                viewportState.viewportCount = 1;
                viewportState.scissorCount = 1;
                viewportState.pViewports = &viewport;
                viewportState.pScissors = &scissor;

                INFO("setting up rasterizer...");
                // rasterizer
                VkPipelineRasterizationStateCreateInfo rasterizer
                        = {}; // does the actual rasterization and outputs fragments
                rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
                rasterizer.depthClampEnable = VK_FALSE; // if true, fragments beyond the near and far planes are clamped
                                                        // instead of discarded
                rasterizer.rasterizerDiscardEnable = VK_FALSE; // if true, geometry never passes through the rasterizer
                                                               // stage(nothing it put into the frame buffer)
                rasterizer.polygonMode = VK_POLYGON_MODE_FILL; // fill the area of the polygon with fragments
                rasterizer.lineWidth = 1.0f;                   // thickness of lines in terms of number of fragments
                // rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;    // cull back faces
                rasterizer.cullMode = VK_CULL_MODE_NONE; // don't cull any faces
                rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
                rasterizer.depthBiasEnable = VK_FALSE;     // depth biasing
                rasterizer.depthBiasConstantFactor = 0.0f; // optional
                rasterizer.depthBiasClamp = 0.0f;          // optional
                rasterizer.depthBiasSlopeFactor = 0.0f;    // optional

                INFO("setting up multisampling...");
                // Multi-sampling
                VkPipelineMultisampleStateCreateInfo multisampling{};
                multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
                multisampling.sampleShadingEnable = VK_FALSE;
                multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
                multisampling.minSampleShading = 1.0f;          // Optional
                multisampling.pSampleMask = nullptr;            // Optional
                multisampling.alphaToCoverageEnable = VK_FALSE; // Optional
                multisampling.alphaToOneEnable = VK_FALSE;      // Optional

                // depth buffering
                // we're not there yet, so we'll disable it for now

                // color blending - combining color from the fragment shader with color that is already in the
                // framebuffer

                INFO("setting up color blending...");
                // controls configuration per attached frame buffer
                // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkBlendOp.html
                // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkBlendFactor.html
                VkPipelineColorBlendAttachmentState colorBlendAttachment{};
                colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT
                                                      | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
                colorBlendAttachment.blendEnable = VK_FALSE;                     // set to true to enable blending
                colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;  // Optional
                colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
                colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;             // Optional
                colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;  // Optional
                colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
                colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;             // Optional

                // global color blending settings
                VkPipelineColorBlendStateCreateInfo colorBlending{};
                colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
                colorBlending.logicOpEnable = VK_FALSE;   // set to true to enable blending
                colorBlending.logicOp = VK_LOGIC_OP_COPY; // Optional
                colorBlending.attachmentCount = 1;
                colorBlending.pAttachments = &colorBlendAttachment;
                colorBlending.blendConstants[0] = 0.0f; // Optional
                colorBlending.blendConstants[1] = 0.0f; // Optional
                colorBlending.blendConstants[2] = 0.0f; // Optional
                colorBlending.blendConstants[3] = 0.0f; // Optional

                INFO("setting up pipeline layout...");
                // pipeline layout - controlling uniform values
                VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
                pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
                pipelineLayoutInfo.setLayoutCount = 1;
                pipelineLayoutInfo.pSetLayouts = &m.descriptorSetLayout; // bind our own layout
                pipelineLayoutInfo.pushConstantRangeCount = 0;          // Optional
                pipelineLayoutInfo.pPushConstantRanges = nullptr;       // Optional

                if (vkCreatePipelineLayout(logicalDevice, &pipelineLayoutInfo, nullptr, &m.pipelineLayout)
                    != VK_SUCCESS) {
                        FATAL("Failed to create pipeline layout!");
                }

                // put things together
                VkGraphicsPipelineCreateInfo pipelineInfo{};
                pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
                pipelineInfo.stageCount = 2;
                pipelineInfo.pStages = shaderStages;
                pipelineInfo.pVertexInputState = &vertexInputInfo;
                pipelineInfo.pInputAssemblyState = &inputAssembly;
                pipelineInfo.pViewportState = &viewportState;
                pipelineInfo.pRasterizationState = &rasterizer;
                pipelineInfo.pMultisampleState = &multisampling;
                pipelineInfo.pDepthStencilState = nullptr; // Optional
                pipelineInfo.pColorBlendState = &colorBlending;
                pipelineInfo.pDynamicState = &dynamicStateCreateInfo;
                pipelineInfo.pDepthStencilState = &depthStencilCreateInfo;
                pipelineInfo.layout = m.pipelineLayout;
                pipelineInfo.renderPass = renderPass;
                pipelineInfo.subpass = 0;

                // only need to specify when deriving from an existing pipeline
                pipelineInfo.basePipelineHandle = VK_NULL_HANDLE; // Optional
                pipelineInfo.basePipelineIndex = -1;              // Optional

                if (vkCreateGraphicsPipelines(
                            logicalDevice, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m.pipeline
                    )
                    != VK_SUCCESS) {
                        FATAL("Failed to create graphics pipeline!");
                }

                vkDestroyShaderModule(logicalDevice, fragShaderModule, nullptr);
                vkDestroyShaderModule(logicalDevice, vertShaderModule, nullptr);
        }

        return m;
}