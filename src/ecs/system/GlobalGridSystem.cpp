#include "GlobalGridSystem.h"
#include "components/Camera.h"
#include "components/Profiler.h"
#include "components/ShaderUtils.h"
#include "components/VulkanUtils.h"
#include "lib/VQDevice.h"
#include "lib/VQUtils.h"

// FIXME: too much repetitive code, graphics pipeline creation can be
// encapsulated
void GlobalGridSystem::createGraphicsPipeline(const VkRenderPass renderPass, const InitContext* initData) {
    /////  ---------- descriptor ---------- /////
    VkDescriptorSetLayoutBinding uboStaticBinding{};
    { // UBO static -- vertex
        uboStaticBinding.binding = (int)BindingLocation::UBO_STATIC;
        uboStaticBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        uboStaticBinding.descriptorCount = 1; // number of values in the array
        uboStaticBinding.stageFlags
            = VK_SHADER_STAGE_VERTEX_BIT; // only used in vertex shader
        uboStaticBinding.pImmutableSamplers = nullptr; // Optional
    }
    VkDescriptorSetLayoutBinding engineUboStaticBinding{};
    { // Engine UBO Static -- vertex
        engineUboStaticBinding.binding = (int)BindingLocation::UBO_STATIC_ENGINE;
        engineUboStaticBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        engineUboStaticBinding.descriptorCount = 1; // number of values in the array
        engineUboStaticBinding.stageFlags
            = VK_SHADER_STAGE_VERTEX_BIT; // only used in vertex shader
        engineUboStaticBinding.pImmutableSamplers = nullptr; // Optional
    }

    std::array<VkDescriptorSetLayoutBinding, 2> bindings = {uboStaticBinding, engineUboStaticBinding};

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
      // FIXME: a global descriptor pool in InitData should suffice
        uint32_t numDescriptorPerType = 5;
        VkDescriptorPoolSize poolSizes[]
            = {{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                static_cast<uint32_t>(numDescriptorPerType)}};

        VkDescriptorPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        poolInfo.poolSizeCount
            = sizeof(poolSizes)
              / sizeof(VkDescriptorPoolSize); // number of pool sizes
        poolInfo.pPoolSizes = poolSizes;
        poolInfo.maxSets = NUM_FRAME_IN_FLIGHT * poolInfo.poolSizeCount;
        poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;

        if (vkCreateDescriptorPool(
                _device->logicalDevice, &poolInfo, nullptr, &_descriptorPool
            )
            != VK_SUCCESS) {
            FATAL("Failed to create descriptor pool!");
        }
    }

    { // _descriptorSets
        // each frame in flight needs its own descriptorset
        std::vector<VkDescriptorSetLayout> layouts(
            NUM_FRAME_IN_FLIGHT, this->_descriptorSetLayout
        );

        VkDescriptorSetAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool = this->_descriptorPool;
        allocInfo.descriptorSetCount = NUM_FRAME_IN_FLIGHT;
        allocInfo.pSetLayouts = layouts.data();

        ASSERT(this->_descriptorSets.size() == NUM_FRAME_IN_FLIGHT);

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
            sizeof(GlobalGridSystem::UBOStatic),
            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
                | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            UBO.buffer
        );
    }

    // update descriptor sets
    // note here we only update descripto sets for static ubo
    // dynamic ubo is updated through `resizeDynamicUbo`
    // texture array is updated through `updateTextureDescriptorSet`
    for (size_t i = 0; i < NUM_FRAME_IN_FLIGHT; i++) {
        VkDescriptorBufferInfo descriptorBufferInfo_static{};
        descriptorBufferInfo_static.buffer = this->_UBO[i].buffer.buffer;
        descriptorBufferInfo_static.offset = 0;
        descriptorBufferInfo_static.range = sizeof(GlobalGridSystem::UBOStatic);

        std::array<VkWriteDescriptorSet, 2> descriptorWrites{};

        descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[0].dstSet = this->_descriptorSets[i];
        descriptorWrites[0].dstBinding = (int)BindingLocation::UBO_STATIC;
        descriptorWrites[0].dstArrayElement = 0;
        descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptorWrites[0].descriptorCount = 1;
        descriptorWrites[0].pBufferInfo = &descriptorBufferInfo_static;


        descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[1].dstSet = this->_descriptorSets[i];
        descriptorWrites[1].dstBinding = (int)BindingLocation::UBO_STATIC_ENGINE;
        descriptorWrites[1].dstArrayElement = 0;
        descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptorWrites[1].descriptorCount = 1;
        descriptorWrites[1].pBufferInfo = &initData->engineUBOStaticDescriptorBufferInfo[i];

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
    auto attributeDescriptions = Vertex::GetAttributeDescriptions();
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
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_LINE_LIST; // draw lines
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
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor = {}; // clips pixels outside the region

    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.scissorCount = 1;
    viewportState.pViewports = &viewport;
    viewportState.pScissors = &scissor;

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
    pipelineInfo.renderPass = renderPass;
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

void GlobalGridSystem::Tick(const TickContext* tickData) {
    PROFILE_SCOPE(tickData->profiler, "Global grid system tick");
    VkCommandBuffer CB = tickData->graphics.CB;
    VkFramebuffer FB = tickData->graphics.currentFB;
    VkExtent2D FBExt = tickData->graphics.currentFBextend;
    int frameIdx = tickData->graphics.currentFrameInFlight;

    vkCmdBindPipeline(CB, VK_PIPELINE_BIND_POINT_GRAPHICS, _pipeline);
    vkCmdBindDescriptorSets(
        CB,
        VK_PIPELINE_BIND_POINT_GRAPHICS,
        _pipelineLayout,
        0,
        1,
        &_descriptorSets[tickData->graphics.currentFrameInFlight],
        0,
        0
    );

    // update static ubo
    // TODO: observe that view and proj can be shared across different
    // pipelines. can we have just one static ubo for specifically view and
    // proj?
    {
        UBOStatic ubo{
            glm::vec3(0.3, 0.3, 0.3)
        };
        // TODO: a better ubo abstraction using templates?
        memcpy(_UBO[frameIdx].buffer.bufferAddress, &ubo, sizeof(ubo));
    }

    // draw the vertex grids
    VkDeviceSize offsets[] = {0};
    VkBuffer vertexBuffers[] = {_gridMesh.vertexBuffer.buffer};
    vkCmdBindVertexBuffers(CB, 0, 1, vertexBuffers, offsets);

    vkCmdDraw(CB, _numLines * 2, 1, 0, 0);
}

void GlobalGridSystem::Init(const InitContext* initData) {
    this->_device = initData->device;
    this->createGraphicsPipeline(initData->renderPass.mainPass, initData);

    // create vertex + index buffer for the global grid
    std::vector<Vertex> vertices;
    const float gridSize = 100.0f; // Total size of the grid (-100 to +100)
    const float gridStep = 1.0f;   // Distance between grid lines
    const float z = -1.0f;         // Set z-coordinate to -1

    // Create vertices for horizontal lines
    for (float y = -gridSize; y <= gridSize; y += gridStep) {
        vertices.push_back(Vertex(glm::vec3(-gridSize, y, z)));
        vertices.push_back(Vertex(glm::vec3(gridSize, y, z)));
        _numLines++;
    }

    // Create vertices for vertical lines
    for (float x = -gridSize; x <= gridSize; x += gridStep) {
        vertices.push_back(Vertex(glm::vec3{x, -gridSize, z}));
        vertices.push_back(Vertex(glm::vec3{x, gridSize, z}));
        _numLines++;
    }
    VQUtils::createVertexBuffer(vertices, _gridMesh.vertexBuffer, *_device);
}

void GlobalGridSystem::Cleanup() {
    DEBUG("cleaning up");
    for (auto& ubo : _UBO) {
        ubo.buffer.Cleanup();
    }

    vkDestroyPipeline(_device->logicalDevice, _pipeline, nullptr);
    vkDestroyPipelineLayout(_device->logicalDevice, _pipelineLayout, nullptr);

    vkDestroyDescriptorSetLayout(
        _device->logicalDevice, _descriptorSetLayout, nullptr
    );
    vkDestroyDescriptorPool(_device->logicalDevice, _descriptorPool, nullptr);

    _gridMesh.vertexBuffer.Cleanup();
    _gridMesh.indexBuffer.Cleanup();

    DEBUG("clean up finished");
}
