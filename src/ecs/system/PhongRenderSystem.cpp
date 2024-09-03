#include "components/ShaderUtils.h"
#include "components/ShaderUtils.h"
#include "components/VulkanUtils.h"
#include "lib/VQDevice.h"
#include "lib/VQUtils.h"
#include "structs/Vertex.h"

#include "components/Camera.h"
#include "components/TextureManager.h"

#include "PhongRenderSystem.h"
#include "ecs/component/ModelComponent.h"
#include "ecs/component/TransformComponent.h"

void PhongRenderSystem::Init(const InitData* initData) {
    _device = initData->device;
    _textureManager = initData->textureManager;
    // create the phong render pass
    this->createRenderPass(_device, initData->swapChainImageFormat);
    // create graphics pipeline
    this->createGraphicsPipeline();
}

void PhongRenderSystem::Tick(const TickData* tickData) {
    VkCommandBuffer CB = tickData->currentCB;
    VkFramebuffer FB = tickData->currentFB;
    VkExtent2D FBExt = tickData->currentFBextend;
    int frameIdx = tickData->currentFrameInFlight;

    { // begin CB
        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = 0;                  // Optional
        beginInfo.pInheritanceInfo = nullptr; // Optional
        if (vkBeginCommandBuffer(
                CB, &beginInfo
            )
            != VK_SUCCESS) {
            FATAL("Failed to begin recording command buffer!");
        }
    }

    vkCmdBindPipeline(CB, VK_PIPELINE_BIND_POINT_GRAPHICS, _pipeline);

    { // begin phong render pass
        VkRenderPassBeginInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassInfo.renderPass = this->_renderPass;
        renderPassInfo.framebuffer = FB;

        renderPassInfo.renderArea.offset = {0, 0};
        renderPassInfo.renderArea.extent = FBExt;

        std::array<VkClearValue, 2> clearValues{};
        clearValues[0].color = {{0.0f, 0.0f, 0.0f, 1.0f}};
        clearValues[1].depthStencil = {1.0f, 0};

        renderPassInfo.clearValueCount
            = static_cast<uint32_t>(clearValues.size());
        renderPassInfo.pClearValues = clearValues.data();

        vkCmdBeginRenderPass(CB, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
    }

    { // set viewport and scissor
        // TODO: don't need to do this in the render system?
        VkViewport viewport{};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = static_cast<float>(FBExt.width);
        viewport.height = static_cast<float>(FBExt.height);
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
        vkCmdSetViewport(CB, 0, 1, &viewport);

        VkRect2D scissor{};
        scissor.offset = {0, 0};
        scissor.extent = FBExt;
        vkCmdSetScissor(CB, 0, 1, &scissor);
    }

    { // update static ubo with model view matrix
        PhongUBOStatic ubo{
            tickData->mainCamera->GetViewMatrix(), // view mat
            tickData->mainProjectionMatrix         // proj mat
        };

        memcpy(_UBO[frameIdx].staticUBO.bufferAddress, &ubo, sizeof(ubo));
    }

    // FIXME: make this into a field.
    static int dynamicAlignment = getDynamicUBOAlignmentSize();
    // loop through entities and render them
    for (Entity* entity : this->_entities) {
        PhongMeshInstanceComponent* meshInstance
            = entity->GetComponent<PhongMeshInstanceComponent>();
        TransformComponent* transform
            = entity->GetComponent<TransformComponent>();
        ASSERT(meshInstance != nullptr)
        ASSERT(transform != nullptr)
        // actual render logic

        uint32_t dynamicUBOOffset
            = meshInstance->dynamicUBOId * dynamicAlignment;
        { // bind descriptor set to the correct dynamic ubo
            // note that we use the same descriptor set for all phong meshes
            // need to rebind because offset to dynamic UBO is different
            vkCmdBindDescriptorSets(
                CB,
                VK_PIPELINE_BIND_POINT_GRAPHICS,
                _pipelineLayout,
                0,
                1,
                &_descriptorSets[tickData->currentFrameInFlight],
                1,
                &dynamicUBOOffset
            );
        }

        { // update dynamic UBO
            // TODO: implement copy in resizeDynamicUbo() so that
            // only need to update dynamic UBO on data change
            void* dynamicUBOAddr = reinterpret_cast<void*>(
                reinterpret_cast<uintptr_t>(
                    _UBO[frameIdx].dynamicUBO.bufferAddress
                )
                + dynamicUBOOffset
            );
            PhongUBODynamic dynamicUBO{
                transform->GetModelMatrix(), meshInstance->textureOffset
            };
            memcpy(dynamicUBOAddr, &dynamicUBO, sizeof(PhongUBODynamic));
        }

        { // bind vertex & index buffer
            VkDeviceSize offsets[] = {0};
            VkBuffer vertexBuffers[]
                = {meshInstance->mesh->vertexBuffer.buffer};
            VkBuffer indexBufffer = meshInstance->mesh->indexBuffer.buffer;
            vkCmdBindVertexBuffers(CB, 0, 1, vertexBuffers, offsets);
            vkCmdBindIndexBuffer(CB, indexBufffer, 0, VK_INDEX_TYPE_UINT32);
        }

        { // issue draw call
            vkCmdDrawIndexed(
                CB, meshInstance->mesh->indexBuffer.numIndices, 1, 0, 0, 0
            );
        }
    }

    vkCmdEndRenderPass(CB); // end phong render pass

    { // end CB
        if (vkEndCommandBuffer(CB) != VK_SUCCESS) {
            FATAL("Failed to record command buffer!");
        }
    }
}

void PhongRenderSystem::createRenderPass(
    VQDevice* device,
    VkFormat swapChainImageFormat
) {
    VkAttachmentDescription colorAttachment{};
    colorAttachment.format = swapChainImageFormat;
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;

    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;

    // don't care about stencil
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    // colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    colorAttachment.finalLayout
        = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL; // for imgui

    VkAttachmentReference colorAttachmentRef{};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentDescription depthAttachment{};
    depthAttachment.format
        = VulkanUtils::findDepthFormat(_device->physicalDevice);
    depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    depthAttachment.finalLayout
        = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentReference depthAttachmentRef{};
    depthAttachmentRef.attachment = 1;
    depthAttachmentRef.layout
        = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    // set up subpass

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;
    subpass.pDepthStencilAttachment = &depthAttachmentRef;

    // dependency to make sure that the render pass waits for the image to be
    // available before drawing
    VkSubpassDependency dependency{};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
                              | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
                              | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT
                               | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

    std::array<VkAttachmentDescription, 2> attachments
        = {colorAttachment, depthAttachment};

    VkRenderPassCreateInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
    renderPassInfo.pAttachments = attachments.data();
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies = &dependency;

    if (vkCreateRenderPass(
            _device->logicalDevice, &renderPassInfo, nullptr, &this->_renderPass
        )
        != VK_SUCCESS) {
        FATAL("Failed to create render pass!");
    }
}

void PhongRenderSystem::createGraphicsPipeline() {

    /////  ---------- descriptor ---------- /////
    VkDescriptorSetLayoutBinding uboStaticBinding{};
    VkDescriptorSetLayoutBinding uboDynamicBinding{};
    VkDescriptorSetLayoutBinding samplerLayoutBinding{};
    VkDescriptorSetLayoutBinding textureArrayLayoutBinding{};
    { // UBO static -- vertex
        uboStaticBinding.binding = (int)BindingLocation::UBO_STATIC;
        uboStaticBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        uboStaticBinding.descriptorCount = 1; // number of values in the array
        uboStaticBinding.stageFlags
            = VK_SHADER_STAGE_VERTEX_BIT; // only used in vertex shader
        uboStaticBinding.pImmutableSamplers = nullptr; // Optional
    }
    { // UBO dynamic -- vertex
        uboDynamicBinding.binding = (int)BindingLocation::UBO_DYNAMIC;
        uboDynamicBinding.descriptorType
            = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
        uboDynamicBinding.descriptorCount = 1; // number of values in the array

        uboDynamicBinding.stageFlags
            = VK_SHADER_STAGE_VERTEX_BIT; // only used in vertex shader
        uboDynamicBinding.pImmutableSamplers = nullptr; // Optional
    }
    { // combined image sampler -- fragment
        samplerLayoutBinding.binding = (int)BindingLocation::TEXTURE_SAMPLER;
        samplerLayoutBinding.descriptorCount = TEXTURE_ARRAY_SIZE;
        samplerLayoutBinding.descriptorType
            = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        samplerLayoutBinding.stageFlags
            = VK_SHADER_STAGE_FRAGMENT_BIT; // only used on fragment shader;
                                            // (may use for vertex shader for
                                            // height mapping)
        samplerLayoutBinding.pImmutableSamplers = nullptr;
    }

    std::array<VkDescriptorSetLayoutBinding, 3> bindings
        = {uboStaticBinding, uboDynamicBinding, samplerLayoutBinding};

    { // _descriptorSetLayout
        VkDescriptorSetLayoutCreateInfo layoutInfo{};
        layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layoutInfo.bindingCount = bindings.size(); // number of bindings
        layoutInfo.pBindings = bindings.data();

        if (vkCreateDescriptorSetLayout(
                _device->logicalDevice,
                &layoutInfo,
                nullptr,
                &this->_descriptorSetLayout
            )
            != VK_SUCCESS) {
            FATAL("Failed to create descriptor set layout!");
        }
    }

    { // _descriptorPool
        uint32_t numDescriptorPerType
            = 5000; // each render group has its own set of descriptors
        VkDescriptorPoolSize poolSizes[]
            = {{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                static_cast<uint32_t>(numDescriptorPerType)},
               {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC,
                static_cast<uint32_t>(numDescriptorPerType)},
               {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                static_cast<uint32_t>(numDescriptorPerType)}};

        VkDescriptorPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        poolInfo.poolSizeCount
            = sizeof(poolSizes)
              / sizeof(VkDescriptorPoolSize); // number of pool sizes
        poolInfo.pPoolSizes = poolSizes;
        poolInfo.maxSets = NUM_INTERMEDIATE_FRAMES * poolInfo.poolSizeCount;
        poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;

        if (vkCreateDescriptorPool(
                _device->logicalDevice, &poolInfo, nullptr, &_descriptorPool
            )
            != VK_SUCCESS) {
            FATAL("Failed to create descriptor pool!");
        }
    }

    { // _descriptorSets
        std::vector<VkDescriptorSetLayout> layouts(
            NUM_INTERMEDIATE_FRAMES, this->_descriptorSetLayout
        );

        VkDescriptorSetAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool = this->_descriptorPool;
        allocInfo.descriptorSetCount = NUM_INTERMEDIATE_FRAMES;
        allocInfo.pSetLayouts = layouts.data();

        ASSERT(this->_descriptorSets.size() == NUM_INTERMEDIATE_FRAMES);

        if (vkAllocateDescriptorSets(
                this->_device->logicalDevice,
                &allocInfo,
                this->_descriptorSets.data()
            )
            != VK_SUCCESS) {
            FATAL("Failed to allocate descriptor sets!");
        }
    }

    /////  ---------- UBO ---------- /////

    // allocate for static ubo, this only needs to be done once
    for (auto& UBO : this->_UBO) {
        _device->CreateBufferInPlace(
            sizeof(PhongUBOStatic),
            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
                | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            UBO.staticUBO
        );
    }

    // allocate for dynamic ubo, and write to descriptors
    resizeDynamicUbo(10);

    // update descriptor sets
    // note here we only update descripto sets for static ubo
    // dynamic ubo is updated through `resizeDynamicUbo`
    // texture array is updated through `updateTextureDescriptorSet`
    for (size_t i = 0; i < NUM_INTERMEDIATE_FRAMES; i++) {
        VkDescriptorBufferInfo descriptorBufferInfo_static{};
        descriptorBufferInfo_static.buffer = this->_UBO[i].staticUBO.buffer;
        descriptorBufferInfo_static.offset = 0;
        descriptorBufferInfo_static.range = sizeof(PhongUBOStatic);

        std::array<VkWriteDescriptorSet, 1> descriptorWrites{};

        descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[0].dstSet = this->_descriptorSets[i];
        descriptorWrites[0].dstBinding = (int)BindingLocation::UBO_STATIC;
        descriptorWrites[0].dstArrayElement = 0;
        descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptorWrites[0].descriptorCount = 1;
        descriptorWrites[0].pBufferInfo = &descriptorBufferInfo_static;

        vkUpdateDescriptorSets(
            _device->logicalDevice,
            descriptorWrites.size(),
            descriptorWrites.data(),
            0,
            nullptr
        );
    }

    /////  ---------- shader ---------- /////

    VkShaderModule vertShaderModule = ShaderCreation::createShaderModule(
        _device->logicalDevice, VERTEX_SHADER_SRC
    );
    VkShaderModule fragShaderModule = ShaderCreation::createShaderModule(
        _device->logicalDevice, FRAGMENT_SHADER_SRC
    );

    VkPipelineShaderStageCreateInfo vertShaderStageInfo = {};
    vertShaderStageInfo.sType
        = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertShaderStageInfo.module = vertShaderModule;
    vertShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo fragShaderStageInfo = {};
    fragShaderStageInfo.sType
        = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragShaderStageInfo.module = fragShaderModule;
    fragShaderStageInfo.pName = "main";

    // static stages(vertex and frag shader)
    VkPipelineShaderStageCreateInfo shaderStages[]
        = {vertShaderStageInfo, fragShaderStageInfo
        }; // put the 2 stages together.

    VkPipelineVertexInputStateCreateInfo vertexInputInfo
        = {}; // describes the format of the vertex data.

    // set up vertex descriptions
    VkVertexInputBindingDescription bindingDescription
        = Vertex::GetBindingDescription();
    std::unique_ptr<std::vector<VkVertexInputAttributeDescription>>
        attributeDescriptions = Vertex::GetAttributeDescriptions();
    {
        vertexInputInfo.sType
            = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

        vertexInputInfo.vertexBindingDescriptionCount = 1;
        vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
        vertexInputInfo.vertexAttributeDescriptionCount
            = static_cast<uint32_t>(attributeDescriptions->size());
        vertexInputInfo.pVertexAttributeDescriptions
            = attributeDescriptions->data();
    }

    // Input assembly
    VkPipelineInputAssemblyStateCreateInfo inputAssembly
        = {}; // describes what kind of geometry will be drawn from the
              // vertices and if primitive restart should be enabled.
    inputAssembly.sType
        = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology
        = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;       // draw triangles
    inputAssembly.primitiveRestartEnable = VK_FALSE; // don't restart primitives

    // depth and stencil state
    VkPipelineDepthStencilStateCreateInfo depthStencilCreateInfo{};
    depthStencilCreateInfo.sType
        = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencilCreateInfo.depthTestEnable = VK_TRUE;
    depthStencilCreateInfo.depthWriteEnable = VK_TRUE;

    depthStencilCreateInfo.depthCompareOp
        = VK_COMPARE_OP_LESS; // low depth == closer object -> cull far
                              // object

    depthStencilCreateInfo.depthBoundsTestEnable = VK_FALSE;
    depthStencilCreateInfo.minDepthBounds = 0.0f; // Optional
    depthStencilCreateInfo.maxDepthBounds = 1.0f; // Optional

    depthStencilCreateInfo.stencilTestEnable = VK_FALSE;
    depthStencilCreateInfo.front = {}; // Optional
    depthStencilCreateInfo.back = {};  // Optional
                                       //

    // dynamic states
    // specifying the dynamic states here so that they are not defined
    // during the pipeline creation, they can be changed at draw time.
    std::vector<VkDynamicState> dynamicStates
        = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};

    INFO("setting up viewport and scissors...");
    VkPipelineDynamicStateCreateInfo dynamicStateCreateInfo = {};
    dynamicStateCreateInfo.sType
        = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicStateCreateInfo.dynamicStateCount
        = static_cast<uint32_t>(dynamicStates.size());
    dynamicStateCreateInfo.pDynamicStates = dynamicStates.data();

    // viewport and scissors
    VkViewport viewport = {}; // describes the region of the framebuffer
                              // that the output will be rendered to.
    // viewport.x = 0.0f;
    // viewport.y = 0.0f;
    // configure viewport resolution to match the swapchain
    // viewport.width = static_cast<float>(swapchainExtent.width);
    // viewport.height = static_cast<float>(swapchainExtent.height);
    // don't know what they are for, but keep them at 0.0f and 1.0f
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor = {}; // clips pixels outside the region
    // scissor.offset = {0, 0};
    // scissor.extent
    //         = swapchainExtent; // we don't want to clip anything, so we
    //         use the swapchain extent

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
    rasterizer.sType
        = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable
        = VK_FALSE; // if true, fragments beyond the near and far planes are
                    // clamped instead of discarded
    rasterizer.rasterizerDiscardEnable
        = VK_FALSE; // if true, geometry never passes through the rasterizer
                    // stage(nothing it put into the frame buffer)
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL; // fill the area of the
                                                   // polygon with fragments
    rasterizer.lineWidth
        = 1.0f; // thickness of lines in terms of number of fragments
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
    multisampling.sType
        = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    multisampling.minSampleShading = 1.0f;          // Optional
    multisampling.pSampleMask = nullptr;            // Optional
    multisampling.alphaToCoverageEnable = VK_FALSE; // Optional
    multisampling.alphaToOneEnable = VK_FALSE;      // Optional

    // depth buffering
    // we're not there yet, so we'll disable it for now

    // color blending - combining color from the fragment shader with color
    // that is already in the framebuffer

    INFO("setting up color blending...");
    // controls configuration per attached frame buffer
    // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkBlendOp.html
    // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkBlendFactor.html
    VkPipelineColorBlendAttachmentState colorBlendAttachment{};
    colorBlendAttachment.colorWriteMask
        = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT
          | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable
        = VK_FALSE; // set to true to enable blending
    colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;  // Optional
    colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
    colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;             // Optional
    colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;  // Optional
    colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
    colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;             // Optional

    // global color blending settings
    VkPipelineColorBlendStateCreateInfo colorBlending{};
    colorBlending.sType
        = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
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
    pipelineLayoutInfo.pSetLayouts
        = &_descriptorSetLayout;                      // bind our own layout
    pipelineLayoutInfo.pushConstantRangeCount = 0;    // Optional
    pipelineLayoutInfo.pPushConstantRanges = nullptr; // Optional

    if (vkCreatePipelineLayout(
            _device->logicalDevice,
            &pipelineLayoutInfo,
            nullptr,
            &_pipelineLayout
        )
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
    pipelineInfo.layout = _pipelineLayout;
    pipelineInfo.renderPass = _renderPass;
    pipelineInfo.subpass = 0;

    // only need to specify when deriving from an existing pipeline
    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE; // Optional
    pipelineInfo.basePipelineIndex = -1;              // Optional

    if (vkCreateGraphicsPipelines(
            _device->logicalDevice,
            VK_NULL_HANDLE,
            1,
            &pipelineInfo,
            nullptr,
            &_pipeline
        )
        != VK_SUCCESS) {
        FATAL("Failed to create graphics pipeline!");
    }

    vkDestroyShaderModule(_device->logicalDevice, fragShaderModule, nullptr);
    vkDestroyShaderModule(_device->logicalDevice, vertShaderModule, nullptr);
}

PhongMeshInstanceComponent* PhongRenderSystem::MakePhongMeshInstanceComponent(
    const std::string& meshPath,
    const std::string& texturePath
) {
    PhongMesh* mesh = nullptr;
    size_t dynamicUBOId = 0;
    int textureOffset = 0;

    { // load or create new mesh
        auto it = _meshes.find(meshPath);
        if (it == _meshes.end()) { // construct phong mesh
            PhongMesh newMesh;
            VQDevice& device = *_device;
            VQUtils::meshToBuffer(
                meshPath.c_str(),
                device,
                newMesh.vertexBuffer,
                newMesh.indexBuffer
            );
            auto result = _meshes.insert({meshPath, newMesh});
            ASSERT(result.second)
            mesh = &(result.first->second);
        } else {
            mesh = &it->second;
        }
    }

    { // load or create new texture
        auto it = _textureDescriptorIndices.find(texturePath);
        if (it == _textureDescriptorIndices.end()) {
            // load texture into textures[textureOffset]
            DEBUG("loading texture into {}", _textureDescriptorInfoIdx);
            _textureManager->GetDescriptorImageInfo(
                texturePath, _textureDescriptorInfo[_textureDescriptorInfoIdx]
            );
            textureOffset = _textureDescriptorInfoIdx;
            _textureDescriptorInfoIdx++;
            updateTextureDescriptorSet();
        } else {
            textureOffset = it->second;
        }
    }

    // reserve a dynamic UBO for this instance
    // note each instance has to have a dynamic ubo, for storing instance data
    // such as texture index and model mat
    // FIXME: on component cleanup, the dynamic ubo should be flagged as free
    // implement something like a CleanUpPhongMeshInstanceComponent()
    {
        dynamicUBOId =_currDynamicUBO;
        _currDynamicUBO++;
        if (_currDynamicUBO >= _numDynamicUBO) {
            resizeDynamicUbo(_numDynamicUBO * 1.5);
        }
    }

    // return new component
    PhongMeshInstanceComponent* ret = new PhongMeshInstanceComponent();
    ret->mesh = mesh;
    ret->dynamicUBOId = dynamicUBOId;
    ret->textureOffset = textureOffset;
    return ret;
}

// reallocate dynamic UBO array, updating the descriptors as well
// note that contents from the old UBO array aren't copied over
void PhongRenderSystem::resizeDynamicUbo(
    size_t dynamicUboCount
) {
    size_t dynamicUboSize = getDynamicUBOAlignmentSize();
    for (int i = 0; i < NUM_INTERMEDIATE_FRAMES; i++) {
        // reallocate dyamic ubo
        this->_UBO[i].dynamicUBO.Cleanup();
        this->_device->CreateBufferInPlace(
            dynamicUboSize * dynamicUboCount,
            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
                | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            this->_UBO[i].dynamicUBO
        );
        // point descriptor to newly allocated buffer
        VkDescriptorBufferInfo descriptorBufferInfo_dynamic{};
        descriptorBufferInfo_dynamic.buffer = this->_UBO[i].dynamicUBO.buffer;
        descriptorBufferInfo_dynamic.offset = 0;
        descriptorBufferInfo_dynamic.range = dynamicUboSize;

        std::array<VkWriteDescriptorSet, 1> descriptorWrites{};
        descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[0].dstSet = this->_descriptorSets[i];
        descriptorWrites[0].dstBinding = (int)BindingLocation::UBO_DYNAMIC;
        descriptorWrites[0].dstArrayElement = 0;
        descriptorWrites[0].descriptorType
            = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
        descriptorWrites[0].descriptorCount = 1;
        descriptorWrites[0].pBufferInfo = &descriptorBufferInfo_dynamic;

        vkUpdateDescriptorSets(
            _device->logicalDevice,
            descriptorWrites.size(),
            descriptorWrites.data(),
            0,
            nullptr
        );
    }
    _numDynamicUBO = dynamicUboCount;
}

void PhongRenderSystem::updateTextureDescriptorSet() {
    DEBUG("updating texture descirptor set");
    for (size_t i = 0; i < NUM_INTERMEDIATE_FRAMES; i++) {
        std::array<VkWriteDescriptorSet, 1> descriptorWrites{};
        descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[0].dstSet = this->_descriptorSets[i];
        descriptorWrites[0].dstBinding = (int)BindingLocation::TEXTURE_SAMPLER;
        descriptorWrites[0].dstArrayElement = 0;
        descriptorWrites[0].descriptorType
            = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        descriptorWrites[0].descriptorCount = _textureDescriptorInfoIdx; // descriptors are 0-indexed, +1 for the # of valid samplers
        descriptorWrites[0].pImageInfo = _textureDescriptorInfo.data();

        DEBUG("descriptor cont: {}", descriptorWrites[0].descriptorCount);

        vkUpdateDescriptorSets(
            _device->logicalDevice,
            descriptorWrites.size(),
            descriptorWrites.data(),
            0,
            nullptr
        );
    }
};

int PhongRenderSystem::getDynamicUBOAlignmentSize() {
    // figure out actual alignment of dynamic UBO
    size_t dynamicAlignment = sizeof(PhongUBODynamic);
    VkPhysicalDeviceProperties properties;
    vkGetPhysicalDeviceProperties(_device->physicalDevice, &properties);
    size_t minUboAlignment = properties.limits.minUniformBufferOffsetAlignment;

    if (minUboAlignment > 0) {
        size_t remainder = dynamicAlignment % minUboAlignment;
        if (remainder > 0) {
            size_t padding = minUboAlignment - remainder;
            dynamicAlignment += padding;
        }
    }
    return dynamicAlignment;
}
