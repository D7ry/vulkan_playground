#include "TriangleApp.h"
#include "utils/ShaderUtils.h"
#include <vulkan/vulkan_core.h>

void TriangleApp::createGraphicsPipeline() {
        INFO("Creating TriangleAPP Graphics Pipeline...");

        INFO("setting up shader modules...");
        // programmable stages
        VkShaderModule vertShaderModule = ShaderCreation::createShaderModule(this->_logicalDevice, "../shaders/vert_test.vert.spv");
        VkShaderModule fragShaderModule = ShaderCreation::createShaderModule(this->_logicalDevice, "../shaders/frag_test.frag.spv");

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
        VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo}; // put the 2 stages together.

        INFO("setting up vertex input bindings...");
        // the configurations of these values will be ignored and they will be specified at draw time;
        // vertex input bindings
        VkPipelineVertexInputStateCreateInfo vertexInputInfo = {}; // describes the format of the vertex data.
        vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        // no descriptions for now, we're experimenting
        vertexInputInfo.vertexBindingDescriptionCount = 0;
        vertexInputInfo.pVertexBindingDescriptions = nullptr;
        vertexInputInfo.vertexAttributeDescriptionCount = 0;
        vertexInputInfo.pVertexAttributeDescriptions = nullptr;

        // Input assembly
        VkPipelineInputAssemblyStateCreateInfo inputAssembly =
            {}; // describes what kind of geometry will be drawn from the vertices and if primitive restart should be enabled.
        inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST; // draw triangles
        inputAssembly.primitiveRestartEnable = VK_FALSE;              // don't restart primitives

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
        VkViewport viewport = {}; // describes the region of the framebuffer that the output will be rendered to.
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        // configure viewport resolution to match the swapchain
        viewport.width = static_cast<float>(this->_swapChainExtent.width);
        viewport.height = static_cast<float>(this->_swapChainExtent.height);
        // don't know what they are for, but keep them at 0.0f and 1.0f
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;

        VkRect2D scissor = {}; // clips pixels outside the region
        scissor.offset = {0, 0};
        scissor.extent = this->_swapChainExtent; // we don't want to clip anything, so we use the swapchain extent

        VkPipelineViewportStateCreateInfo viewportState{};
        viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewportState.viewportCount = 1;
        viewportState.scissorCount = 1;
        viewportState.pViewports = &viewport;
        viewportState.pScissors = &scissor;

        INFO("setting up rasterizer...");
        // rasterizer
        VkPipelineRasterizationStateCreateInfo rasterizer = {}; // does the actual rasterization and outputs fragments
        rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterizer.depthClampEnable = VK_FALSE; // if true, fragments beyond the near and far planes are clamped instead of discarded
        rasterizer.rasterizerDiscardEnable =
            VK_FALSE; // if true, geometry never passes through the rasterizer stage(nothing it put into the frame buffer)
        rasterizer.polygonMode = VK_POLYGON_MODE_FILL;  // fill the area of the polygon with fragments
        rasterizer.lineWidth = 1.0f;                    // thickness of lines in terms of number of fragments
        rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;    // cull back faces
        rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE; // vertex order for faces to be considered front-facing
        rasterizer.depthBiasEnable = VK_FALSE;          // depth biasing
        rasterizer.depthBiasConstantFactor = 0.0f;      // optional
        rasterizer.depthBiasClamp = 0.0f;               // optional
        rasterizer.depthBiasSlopeFactor = 0.0f;         // optional

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

        // color blending - combining color from the fragment shader with color that is already in the framebuffer

        INFO("setting up color blending...");
        // controls configuration per attached frame buffer
        // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkBlendOp.html
        // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkBlendFactor.html
        VkPipelineColorBlendAttachmentState colorBlendAttachment{};
        colorBlendAttachment.colorWriteMask =
            VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
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
        pipelineLayoutInfo.setLayoutCount = 0;            // Optional
        pipelineLayoutInfo.pSetLayouts = nullptr;         // Optional
        pipelineLayoutInfo.pushConstantRangeCount = 0;    // Optional
        pipelineLayoutInfo.pPushConstantRanges = nullptr; // Optional

        if (vkCreatePipelineLayout(_logicalDevice, &pipelineLayoutInfo, nullptr, &this->_pipelineLayout) != VK_SUCCESS) {
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
        pipelineInfo.layout = this->_pipelineLayout;
        pipelineInfo.renderPass = this->_renderPass;
        pipelineInfo.subpass = 0;

        // only need to specify when deriving from an existing pipeline
        pipelineInfo.basePipelineHandle = VK_NULL_HANDLE; // Optional
        pipelineInfo.basePipelineIndex = -1;              // Optional

        if (vkCreateGraphicsPipelines(this->_logicalDevice, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &this->_graphicsPipeline) != VK_SUCCESS) {
                FATAL("Failed to create graphics pipeline!");
        }

        vkDestroyShaderModule(_logicalDevice, fragShaderModule, nullptr);
        vkDestroyShaderModule(_logicalDevice, vertShaderModule, nullptr);
        INFO("Graphics pipeline created.");
}

void TriangleApp::createRenderPass() {
        INFO("Creating render pass...");
        VkAttachmentDescription colorAttachment{};
        colorAttachment.format = this->_swapChainImageFormat;
        colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;

        colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;

        // don't care about stencil
        colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

        colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

        // set up subpass
        VkAttachmentReference colorAttachmentRef{};
        colorAttachmentRef.attachment = 0;
        colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        VkSubpassDescription subpass{};
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount = 1;
        subpass.pColorAttachments = &colorAttachmentRef;

        VkRenderPassCreateInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        renderPassInfo.attachmentCount = 1;
        renderPassInfo.pAttachments = &colorAttachment;
        renderPassInfo.subpassCount = 1;
        renderPassInfo.pSubpasses = &subpass;

        if (vkCreateRenderPass(this->_logicalDevice, &renderPassInfo, nullptr, &this->_renderPass) != VK_SUCCESS) {
                FATAL("Failed to create render pass!");
        }
}
void TriangleApp::createFramebuffers() {
        INFO("Creating {} framebuffers...", this->_swapChainImageViews.size());
        this->_swapChainFrameBuffers.resize(this->_swapChainImageViews.size());
        // iterate through image views and create framebuffers
        for (size_t i = 0; i < _swapChainImageViews.size(); i++) {
                VkImageView atachments[] = {_swapChainImageViews[i]};
                VkFramebufferCreateInfo framebufferInfo{};
                framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
                framebufferInfo.renderPass = _renderPass; // each framebuffer is associated with a render pass;  they need to be compatible i.e.
                                                          // having same number of attachments and same formats
                framebufferInfo.attachmentCount = 1;
                framebufferInfo.pAttachments = atachments;
                framebufferInfo.width = _swapChainExtent.width;
                framebufferInfo.height = _swapChainExtent.height;
                framebufferInfo.layers = 1; // number of layers in image arrays
                if (vkCreateFramebuffer(_logicalDevice, &framebufferInfo, nullptr, &_swapChainFrameBuffers[i]) != VK_SUCCESS) {
                        FATAL("Failed to create framebuffer!");
                }
        }
        INFO("Framebuffers created.");
}

void TriangleApp::createCommandPool() {
        INFO("Creating command pool...");
        QueueFamilyIndices queueFamilyIndices = findQueueFamilies(this->_physicalDevice);

        VkCommandPoolCreateInfo poolInfo {};
        poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT; // ALlow command buffers to be re-recorded individually
        // we want to re-record the command buffer every single frame.
        poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();

        if (vkCreateCommandPool(this->_logicalDevice, &poolInfo, nullptr, &this->_commandPool) != VK_SUCCESS) {
                FATAL("Failed to create command pool!");
        }
        INFO("Command pool created.");
}

void TriangleApp::createCommandBuffer() {
        INFO("Creating command buffer");

        VkCommandBufferAllocateInfo allocInfo {};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.commandPool = this->_commandPool;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandBufferCount = 1;

        if (vkAllocateCommandBuffers(this->_logicalDevice, &allocInfo, &this->_commandBuffer) != VK_SUCCESS) {
                FATAL("Failed to allocate command buffers!");
        }
}

void TriangleApp::recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex) {
        INFO("Recording command buffer...");
        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = 0; // Optional
        beginInfo.pInheritanceInfo = nullptr; // Optional

        if (vkBeginCommandBuffer(this->_commandBuffer, &beginInfo) != VK_SUCCESS) {
                FATAL("Failed to begin recording command buffer!");
        }

        // start render pass
        VkRenderPassBeginInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassInfo.renderPass = this->_renderPass;
        renderPassInfo.framebuffer = this->_swapChainFrameBuffers[imageIndex];

        renderPassInfo.renderArea.offset = {0, 0};
        renderPassInfo.renderArea.extent = this->_swapChainExtent;

        VkClearValue clearColor = {{{0.0f, 0.0f, 0.0f, 1.0f}}}; // black with 100% opacity
        renderPassInfo.clearValueCount = 1;
        renderPassInfo.pClearValues = &clearColor;

        vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

        // bind graphics pipeline
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, this->_graphicsPipeline);

        VkViewport viewport{};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = static_cast<float>(_swapChainExtent.width);
        viewport.height = static_cast<float>(_swapChainExtent.height);
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
        vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

        VkRect2D scissor{};
        scissor.offset = {0, 0};
        scissor.extent = _swapChainExtent;
        vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

        // issue the draw command
        vkCmdDraw(commandBuffer, 3, 1, 0, 0);
        vkCmdEndRenderPass(commandBuffer);

        if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
                FATAL("Failed to record command buffer!");
        }
        INFO("Command buffer recorded.");
}

void TriangleApp::drawFrame() {
       //  Wait for the previous frame to finish
       //  Acquire an image from the swap chain
       //  Record a command buffer which draws the scene onto that image
       //  Submit the recorded command buffer
       //  Present the swap chain image
}