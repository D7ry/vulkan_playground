#include <glm/fwd.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "TriangleApp.h"
#include "structs/UniformBuffer.h"
#include "utils/Animation.h"
#include "utils/ShaderUtils.h"
#include <GLFW/glfw3.h>
#include <cstdint>
#include <glm/trigonometric.hpp>
#include <vulkan/vulkan_core.h>
void TriangleApp::keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
        if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
                INFO("Esc key pressed, closing window...");
                glfwSetWindowShouldClose(window, GLFW_TRUE);
        }
        switch (key) {

        }
}

void TriangleApp::setKeyCallback() { glfwSetKeyCallback(this->_window, TriangleApp::keyCallback); }

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
        VkPipelineVertexInputStateCreateInfo vertexInputInfo = {}; // describes the format of the vertex data.
        vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        // set up vertex descriptions
        VkVertexInputBindingDescription bindingDescription = Vertex::GetBindingDescription();
        std::unique_ptr<std::vector<VkVertexInputAttributeDescription>> attributeDescriptions = Vertex::GetAttributeDescriptions();

        vertexInputInfo.vertexBindingDescriptionCount = 1;
        vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
        vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions->size());
        vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions->data();

        // Input assembly
        VkPipelineInputAssemblyStateCreateInfo inputAssembly
                = {}; // describes what kind of geometry will be drawn from the vertices and if primitive restart should be enabled.
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
        rasterizer.rasterizerDiscardEnable
                = VK_FALSE; // if true, geometry never passes through the rasterizer stage(nothing it put into the frame buffer)
        rasterizer.polygonMode = VK_POLYGON_MODE_FILL;  // fill the area of the polygon with fragments
        rasterizer.lineWidth = 1.0f;                    // thickness of lines in terms of number of fragments
        //rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;    // cull back faces
        rasterizer.cullMode = VK_CULL_MODE_NONE; // don't cull any faces
        rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
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
        colorBlendAttachment.colorWriteMask
                = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
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
        pipelineLayoutInfo.pSetLayouts = &_descriptorSetLayout; // bind our own layout
        pipelineLayoutInfo.pushConstantRangeCount = 0;          // Optional
        pipelineLayoutInfo.pPushConstantRanges = nullptr;       // Optional

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

        // dependency to make sure that the render pass waits for the image to be available before drawing
        VkSubpassDependency dependency{};
        dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
        dependency.dstSubpass = 0;
        dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.srcAccessMask = 0;
        dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

        VkRenderPassCreateInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        renderPassInfo.attachmentCount = 1;
        renderPassInfo.pAttachments = &colorAttachment;
        renderPassInfo.subpassCount = 1;
        renderPassInfo.pSubpasses = &subpass;
        renderPassInfo.dependencyCount = 1;
        renderPassInfo.pDependencies = &dependency;

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

        VkCommandPoolCreateInfo poolInfo{};
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

        this->_commandBuffers.resize(this->MAX_FRAMES_IN_FLIGHT);

        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.commandPool = this->_commandPool;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandBufferCount = (uint32_t)this->_commandBuffers.size();

        if (vkAllocateCommandBuffers(this->_logicalDevice, &allocInfo, this->_commandBuffers.data()) != VK_SUCCESS) {
                FATAL("Failed to allocate command buffers!");
        }
}

void TriangleApp::recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex) { // called every frame
        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = 0;                  // Optional
        beginInfo.pInheritanceInfo = nullptr; // Optional

        if (vkBeginCommandBuffer(this->_commandBuffers[_currentFrame], &beginInfo) != VK_SUCCESS) {
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

        VkBuffer vertexBuffers[] = {this->_vertexBuffer};
        VkDeviceSize offsets[] = {0};
        vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);

        vkCmdBindIndexBuffer(commandBuffer, this->_indexBuffer, 0, VK_INDEX_TYPE_UINT16);

        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, _pipelineLayout, 0, 1, &_descriptorSets[_currentFrame], 0, nullptr);

        // issue the draw command
        // vkCmdDraw(commandBuffer, 3, 1, 0, 0);
        vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(_indices.size()), 1, 0, 0, 0);
        vkCmdEndRenderPass(commandBuffer);

        if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
                FATAL("Failed to record command buffer!");
        }
}

// TODO: inreal world scenarios we do not call vkALlocateMemory for every individual buffer(costly).
//  Instead we should use a custom allocator or use VulkanMemoryAllocator.
void TriangleApp::createVertexBuffer() {
        INFO("Creating vertex buffer...");
        VkDeviceSize vertexBufferSize = sizeof(_vertices[0]) * _vertices.size();

        std::pair<VkBuffer, VkDeviceMemory> res = createStagingBuffer(this, vertexBufferSize);
        VkBuffer stagingBuffer = res.first;
        VkDeviceMemory stagingBufferMemory = res.second;
        // copy over data from cpu memory to gpu memory(staging buffer)
        void* data;
        vkMapMemory(_logicalDevice, stagingBufferMemory, 0, vertexBufferSize, 0, &data);
        memcpy(data, _vertices.data(), (size_t)vertexBufferSize); // copy the data
        vkUnmapMemory(_logicalDevice, stagingBufferMemory);

        // create vertex buffer
        createBuffer(
                _physicalDevice,
                _logicalDevice,
                vertexBufferSize,
                VK_BUFFER_USAGE_TRANSFER_DST_BIT // can be used as destination in a memory transfer operation
                        | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, // local to the GPU for faster access
                _vertexBuffer,
                _vertexBufferMemory
        );

        copyBuffer(_logicalDevice, _commandPool, _graphicsQueue, stagingBuffer, _vertexBuffer, vertexBufferSize);

        // get rid of staging buffer, it is very much temproary
        vkDestroyBuffer(_logicalDevice, stagingBuffer, nullptr);
        vkFreeMemory(_logicalDevice, stagingBufferMemory, nullptr);
}

void TriangleApp::createIndexBuffer() {
        INFO("Creating index buffer...");
        VkDeviceSize indexBufferSize = sizeof(_indices[0]) * _indices.size();

        std::pair<VkBuffer, VkDeviceMemory> res = createStagingBuffer(this, indexBufferSize);
        VkBuffer stagingBuffer = res.first;
        VkDeviceMemory stagingBufferMemory = res.second;
        // copy over data from cpu memory to gpu memory(staging buffer)
        void* data;
        vkMapMemory(_logicalDevice, stagingBufferMemory, 0, indexBufferSize, 0, &data);
        memcpy(data, _indices.data(), (size_t)indexBufferSize); // copy the data
        vkUnmapMemory(_logicalDevice, stagingBufferMemory);

        // create index buffer
        createBuffer(
                _physicalDevice,
                _logicalDevice,
                indexBufferSize,
                VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                _indexBuffer,
                _indexBufferMemory
        );

        copyBuffer(_logicalDevice, _commandPool, _graphicsQueue, stagingBuffer, _indexBuffer, indexBufferSize);

        vkDestroyBuffer(_logicalDevice, stagingBuffer, nullptr);
        vkFreeMemory(_logicalDevice, stagingBufferMemory, nullptr);
}

void TriangleApp::createUniformBuffers() {
        INFO("Creating uniform buffers...");
        VkDeviceSize uniformBufferSize = sizeof(UniformBuffer);

        _uniformBuffers.resize(MAX_FRAMES_IN_FLIGHT);
        _uniformBuffersMemory.resize(MAX_FRAMES_IN_FLIGHT);
        _uniformBuffersData.resize(MAX_FRAMES_IN_FLIGHT);

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
                createBuffer(
                        _physicalDevice,
                        _logicalDevice,
                        uniformBufferSize,
                        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT // uniform buffer is visible to the CPU since it's replaced every frame.
                                | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                        _uniformBuffers[i],
                        _uniformBuffersMemory[i]
                );
                vkMapMemory(_logicalDevice, _uniformBuffersMemory[i], 0, uniformBufferSize, 0, &_uniformBuffersData[i]);
        }
}

void TriangleApp::createDescriptorSetLayout() {
        VkDescriptorSetLayoutBinding uboLayoutBinding{};
        uboLayoutBinding.binding = 0; // binding = 0 in shader
        uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        uboLayoutBinding.descriptorCount = 1; // number of values in the array

        uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT; // only used in vertex shader
        uboLayoutBinding.pImmutableSamplers = nullptr;            // Optional

        VkDescriptorSetLayoutCreateInfo layoutInfo{};
        layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layoutInfo.bindingCount = 1; // number of bindings
        layoutInfo.pBindings = &uboLayoutBinding;

        if (vkCreateDescriptorSetLayout(_logicalDevice, &layoutInfo, nullptr, &_descriptorSetLayout) != VK_SUCCESS) {
                FATAL("Failed to create descriptor set layout!");
        }
}

void TriangleApp::createDescriptorPool() {
        // create a pool that will allocate to actual descriptor sets
        int descriptorSetCount = static_cast<int>(MAX_FRAMES_IN_FLIGHT);
        VkDescriptorPoolSize poolSize{};
        poolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        poolSize.descriptorCount = descriptorSetCount; // each frame has a uniform buffer

        VkDescriptorPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        poolInfo.poolSizeCount = 1; // number of pool sizes
        poolInfo.pPoolSizes = &poolSize;
        poolInfo.maxSets = descriptorSetCount; // number of descriptor sets, set to the number of frames in flight

        if (vkCreateDescriptorPool(_logicalDevice, &poolInfo, nullptr, &_descriptorPool) != VK_SUCCESS) {
                FATAL("Failed to create descriptor pool!");
        }
}

void TriangleApp::createDescriptorSets() {
        int numDescriptorSets = static_cast<int>(MAX_FRAMES_IN_FLIGHT);
        std::vector<VkDescriptorSetLayout> layouts(numDescriptorSets, _descriptorSetLayout);
        VkDescriptorSetAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool = _descriptorPool;
        allocInfo.descriptorSetCount = numDescriptorSets;
        allocInfo.pSetLayouts = layouts.data();

        _descriptorSets.resize(numDescriptorSets);
        if (vkAllocateDescriptorSets(_logicalDevice, &allocInfo, _descriptorSets.data()) != VK_SUCCESS) {
                FATAL("Failed to allocate descriptor sets!");
        }

        for (size_t i = 0; i < numDescriptorSets; i++) {
                VkDescriptorBufferInfo descriptorBufferInfo{};
                descriptorBufferInfo.buffer = _uniformBuffers[i];
                descriptorBufferInfo.offset = 0;
                descriptorBufferInfo.range = sizeof(UniformBuffer);

                VkWriteDescriptorSet descriptorWrite{};
                descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                descriptorWrite.dstSet = _descriptorSets[i];
                descriptorWrite.dstBinding = 0; // zbinding = 0 in shader
                descriptorWrite.dstArrayElement = 0;
                descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                descriptorWrite.descriptorCount = 1;
                descriptorWrite.pBufferInfo = &descriptorBufferInfo;
                descriptorWrite.pImageInfo = nullptr;       // Optional
                descriptorWrite.pTexelBufferView = nullptr; // Optional

                vkUpdateDescriptorSets(_logicalDevice, 1, &descriptorWrite, 0, nullptr);
        }
}

void TriangleApp::updateUniformBufferData(uint32_t frameIndex) {
        static Animation::StopWatchSeconds timer;
        float time = timer.elapsed();
        UniformBuffer ubo{};
        auto initialPos = glm::mat4(1.f);
        ubo.model = initialPos;
        ubo.model = glm::rotate(ubo.model, time * glm::radians(90.f), glm::vec3(0.f, 0.f, 1.f));
        //ubo.model = glm::translate(ubo.model, glm::vec3(0.f, 0.f, time - int(time)));
        ubo.view = glm::lookAt(glm::vec3(2.f, 2.f, 2.f), glm::vec3(0.f, 0.f, 0.f), glm::vec3(0.f, 0.f, 1.f));
        ubo.proj = glm::perspective(glm::radians(30.f), _swapChainExtent.width / (float)_swapChainExtent.height, 0.1f, 10.f);

        ubo.proj[1][1] *= -1; // flip y coordinate
        // TODO: use push constants for small data update
        memcpy(_uniformBuffersData[frameIndex], &ubo, sizeof(ubo));
}

void TriangleApp::drawFrame() {
        //  Wait for the previous frame to finish
        vkWaitForFences(this->_logicalDevice, 1, &this->_fenceInFlight[this->_currentFrame], VK_TRUE, UINT64_MAX);

        //  Acquire an image from the swap chain
        uint32_t imageIndex;
        VkResult result = vkAcquireNextImageKHR(
                this->_logicalDevice, _swapChain, UINT64_MAX, _semaImageAvailable[this->_currentFrame], VK_NULL_HANDLE, &imageIndex
        );
        if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
                this->recreateSwapChain();
                return;
        } else if (result != VK_SUCCESS) {
                FATAL("Failed to acquire swap chain image!");
        }

        // lock the fence
        vkResetFences(this->_logicalDevice, 1, &this->_fenceInFlight[this->_currentFrame]);

        //  Record a command buffer which draws the scene onto that image
        vkResetCommandBuffer(this->_commandBuffers[this->_currentFrame], 0);
        this->recordCommandBuffer(this->_commandBuffers[this->_currentFrame], imageIndex);

        this->updateUniformBufferData(_currentFrame);

        //  Submit the recorded command buffer
        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

        VkSemaphore waitSemaphores[]
                = {_semaImageAvailable[_currentFrame]}; // use imageAvailable semaphore to make sure that the image is available before drawing
        VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
        submitInfo.waitSemaphoreCount = 1;
        submitInfo.pWaitSemaphores = waitSemaphores;
        submitInfo.pWaitDstStageMask = waitStages;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &_commandBuffers[_currentFrame];

        VkSemaphore signalSemaphores[] = {_semaRenderFinished[_currentFrame]};
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = signalSemaphores;

        if (vkQueueSubmit(_graphicsQueue, 1, &submitInfo, _fenceInFlight[_currentFrame]) != VK_SUCCESS) {
                FATAL("Failed to submit draw command buffer!");
        }

        //  Present the swap chain image
        VkPresentInfoKHR presentInfo{};
        presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores = signalSemaphores; // wait for render to finish before presenting

        VkSwapchainKHR swapChains[] = {_swapChain};
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = swapChains;
        presentInfo.pImageIndices = &imageIndex; // specify which image to present
        presentInfo.pResults = nullptr;          // Optional: can be used to check if presentation was successful

        result = vkQueuePresentKHR(_presentationQueue, &presentInfo);
        if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || this->_framebufferResized) {
                this->recreateSwapChain();
                this->_framebufferResized = false;
        } else if (result != VK_SUCCESS) {
                FATAL("Failed to present swap chain image!");
        }
        //  Advance to the next frame
        _currentFrame = (_currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}