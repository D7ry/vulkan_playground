#include "components/Profiler.h"
#include "components/ShaderUtils.h"
#include "components/VulkanUtils.h"
#include "lib/VQDevice.h"
#include "lib/VQUtils.h"
#include "structs/Vertex.h"

#include "components/Camera.h"
#include "components/TextureManager.h"

#include "BindlessRenderSystem.h"
#include "ecs/component/TransformComponent.h"

void BindlessRenderSystem::Init(const InitContext* initData) {
    _device = initData->device;
    _textureManager = initData->textureManager;
    createBindlessResources();
    // create graphics pipeline
    createGraphicsPipeline(initData->renderPass.mainPass, initData);
}

void BindlessRenderSystem::Cleanup() {
    // clean up pipeline
    vkDestroyPipeline(_device->logicalDevice, _pipeline, nullptr);
    vkDestroyPipelineLayout(_device->logicalDevice, _pipelineLayout, nullptr);

    // clean up descriptors
    vkDestroyDescriptorSetLayout(
        _device->logicalDevice, _descriptorSetLayout, nullptr
    );
    // descriptor sets automatically cleaned up
    vkDestroyDescriptorPool(_device->logicalDevice, _descriptorPool, nullptr);

    DEBUG("phong rendering system cleaned up.");
    // note: texture is handled by TextureManager so no need to clean that up
}

void BindlessRenderSystem::Tick(const TickContext* ctx) {
    PROFILE_SCOPE(ctx->profiler, "Bindless System Tick");
    VkCommandBuffer CB = ctx->graphics.CB;
    VkFramebuffer FB = ctx->graphics.currentFB;
    VkExtent2D FBExt = ctx->graphics.currentFBextend;
    int frameIdx = ctx->graphics.currentFrameInFlight;

    vkCmdBindPipeline(CB, VK_PIPELINE_BIND_POINT_GRAPHICS, _pipeline);

    // only use the global engine UBO, so need to bind once only
    vkCmdBindDescriptorSets(
        CB,
        VK_PIPELINE_BIND_POINT_GRAPHICS,
        _pipelineLayout,
        0,
        1,
        &_descriptorSets[frameIdx],
        0,
        0
    );

    // flush buffer updates, write to the buffer corresponding to the current
    // frame
    // TODO: actually implement buffer flushing

    VkDeviceSize offsets[] = {0};
    vkCmdBindVertexBuffers(CB, 0, 1, &_vertexBuffers.buffer, offsets);
    vkCmdBindIndexBuffer(CB, _indexBuffers.buffer, 0, VK_INDEX_TYPE_UINT32);
    vkCmdDrawIndexedIndirect(
        CB,
        _bindlessBuffers[frameIdx].drawCommandArray.buffer,
        0, // offset
        _drawCommandArrayOffset
            / sizeof(VkDrawIndexedIndirectCommand), // drawCount
        sizeof(VkDrawIndexedIndirectCommand)        // stride
    );
}

void BindlessRenderSystem::createGraphicsPipeline(
    const VkRenderPass renderPass,
    const InitContext* initData
) {
    /////  ---------- descriptor ---------- /////
    VkDescriptorSetLayoutBinding uboStaticBinding{};
    VkDescriptorSetLayoutBinding samplerLayoutBinding{};
    VkDescriptorSetLayoutBinding instanceDataArrayBinding{};
    VkDescriptorSetLayoutBinding instanceIndexArrayBinding{};
    { // UBO static -- vertex
        uboStaticBinding.binding = (int)BindingLocation::UBO_STATIC_ENGINE;
        uboStaticBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        uboStaticBinding.descriptorCount = 1; // number of values in the array
        uboStaticBinding.stageFlags
            = VK_SHADER_STAGE_VERTEX_BIT; // only used in vertex shader
        uboStaticBinding.pImmutableSamplers = nullptr; // Optional
    }
    { // instance data array -- vertex
        instanceDataArrayBinding.binding = (int)BindingLocation::INSTANCE_DATA;
        instanceDataArrayBinding.descriptorType
            = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        instanceDataArrayBinding.descriptorCount
            = 1; // number of values in the array
        instanceDataArrayBinding.stageFlags
            = VK_SHADER_STAGE_VERTEX_BIT; // only used in vertex shader
        instanceDataArrayBinding.pImmutableSamplers = nullptr; // Optional
    }
    { // instance index array -- vertex
        instanceIndexArrayBinding.binding
            = (int)BindingLocation::INSTANCE_INDEX;
        instanceIndexArrayBinding.descriptorType
            = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        instanceIndexArrayBinding.descriptorCount
            = 1; // number of values in the array
        instanceIndexArrayBinding.stageFlags
            = VK_SHADER_STAGE_VERTEX_BIT; // only used in vertex shader
        instanceIndexArrayBinding.pImmutableSamplers = nullptr; // Optional
    }
    { // combined image sampler array -- fragment
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

    std::array<VkDescriptorSetLayoutBinding, 4> bindings
        = {uboStaticBinding,
           samplerLayoutBinding,
           instanceDataArrayBinding,
           instanceIndexArrayBinding};

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
            = 50; // each render group has its own set of descriptors
        VkDescriptorPoolSize poolSizes[]
            = {{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                static_cast<uint32_t>(numDescriptorPerType)},
               {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                static_cast<uint32_t>(numDescriptorPerType)},
               {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                static_cast<uint32_t>(5000)}}; // to fit all textures

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

    for (size_t i = 0; i < NUM_FRAME_IN_FLIGHT; i++) {
        std::array<VkWriteDescriptorSet, 3> descriptorWrites{};

        // engine ubo static
        descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[0].dstSet = this->_descriptorSets[i];
        descriptorWrites[0].dstBinding
            = (int)BindingLocation::UBO_STATIC_ENGINE;
        descriptorWrites[0].dstArrayElement = 0;
        descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptorWrites[0].descriptorCount = 1;
        descriptorWrites[0].pBufferInfo
            = &initData->engineUBOStaticDescriptorBufferInfo[i];

        // instance data
        VkDescriptorBufferInfo bufferInfoInstanceData{};
        bufferInfoInstanceData.buffer
            = _bindlessBuffers[i].instanceDataArray.buffer;
        bufferInfoInstanceData.offset = 0;
        bufferInfoInstanceData.range
            = _bindlessBuffers[i].instanceDataArray.size;
        descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[1].dstSet = this->_descriptorSets[i];
        descriptorWrites[1].dstBinding = (int)BindingLocation::INSTANCE_DATA;
        descriptorWrites[1].dstArrayElement = 0;
        descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        descriptorWrites[1].descriptorCount = 1;
        descriptorWrites[1].pBufferInfo = &bufferInfoInstanceData;

        // instance index
        VkDescriptorBufferInfo bufferInfoInstanceIndex{};
        bufferInfoInstanceIndex.buffer
            = _bindlessBuffers[i].instanceLookupArray.buffer;
        bufferInfoInstanceIndex.offset = 0;
        bufferInfoInstanceIndex.range
            = _bindlessBuffers[i].instanceLookupArray.size;
        descriptorWrites[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[2].dstSet = this->_descriptorSets[i];
        descriptorWrites[2].dstBinding = (int)BindingLocation::INSTANCE_INDEX;
        descriptorWrites[2].dstArrayElement = 0;
        descriptorWrites[2].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        descriptorWrites[2].descriptorCount = 1;
        descriptorWrites[2].pBufferInfo = &bufferInfoInstanceIndex;

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

BindlessRenderSystemComponent* BindlessRenderSystem::MakeComponent(
    const std::string& meshPath,
    const std::string& texturePath
) {
    size_t instanceID = 0;
    int textureIndex = 0;
    ASSERT(_textureManager);

    { // load or create new texture
        auto it = _textureDescriptorIndices.find(texturePath);
        if (it == _textureDescriptorIndices.end()) {
            int textureOffset = _textureDescriptorIndices.size();
            // load texture into textures[textureOffset]
            DEBUG("loading {} into {}", texturePath, textureOffset);
            _textureManager->GetDescriptorImageInfo(
                texturePath, _textureDescriptorInfo[textureOffset]
            );
            textureOffset = textureOffset;
            _textureDescriptorInfoIdx++;
            // must update the descriptor set to reflect loading newtexture
            // FIXME: current update is not thread-safe as it writes
            // too all descriptors(including one that's in flight). use an
            // update queue instead.
            updateTextureDescriptorSet();
            _textureDescriptorIndices.insert({texturePath, textureOffset});
        } else {
            textureIndex = it->second;
        }
    }

    // look for a batch to put the instance into.
    // if no batch is available, create a new batch with 1.5x the
    // old batch's size
    auto it = _modelBatches.find(meshPath);
    if (it == _modelBatches.end()) {
        auto res = _modelBatches.insert({meshPath, {}});
        ASSERT(res.second);
        it = res.first;
    }
    RenderBatch* pBatch = nullptr;
    for (RenderBatch& batch : it->second) {
        if (batch.instanceCount < batch.maxSize) {
            pBatch = std::addressof(batch);
            break;
        }
    }

    // didn't find a suitable batch, create a new batch
    if (pBatch == nullptr) {
        // currently use geometric scaling
        int batchSize = 10;
        // each batch is 10x the size of the last
        batchSize *= (it->second.empty() ? 1 : it->second.back().maxSize);
        it->second.push_back(createRenderBatch(meshPath, batchSize));
        pBatch = std::addressof(it->second.back());
    }

    pBatch->instanceCount += 1; // the model now belongs to the batch

    BindlessRenderSystemComponent* ret = new BindlessRenderSystemComponent();
    ret->parentSystem = this;
    ret->instanceDataOffset = _instanceDataArrayOffset;

    // create new instance data, push to instance data array
    BindlessInstanceData data{};
    data.textureIndex.albedo = textureIndex; // TODO: add other texture supports
    data.drawCmdIndex = pBatch->drawCmdOffset
                        / sizeof(VkDrawIndexedIndirectCommand
                        ); // offset divided by size to get index
    data.transparency = 0.f;
    data.model = glm::mat4(1.f); // identity mat
    // push data to the array
    for (int i = 0; i < NUM_FRAME_IN_FLIGHT; i++) {
        char* addr = reinterpret_cast<char*>(
                         _bindlessBuffers[i].instanceDataArray.bufferAddress
                     )
                     + _instanceDataArrayOffset;
        memcpy(addr, std::addressof(data), sizeof(BindlessInstanceData));
    }
    _instanceDataArrayOffset += sizeof(BindlessInstanceData);

    {
        // hack in the process of compute shader culling by
        // 1. inserting the instance index into lookup array manually,
        // 2. modifying the render command's draw number
        for (int i = 0; i < NUM_FRAME_IN_FLIGHT; i++) {
            VkDrawIndexedIndirectCommand cmd;
            // read command from drawCommandArray
            VkDrawIndexedIndirectCommand* pCmd
                = reinterpret_cast<VkDrawIndexedIndirectCommand*>(
                    (char*)_bindlessBuffers[i].drawCommandArray.bufferAddress
                    + pBatch->drawCmdOffset
                );
            unsigned int* pIndex = reinterpret_cast<unsigned int*>(
                (char*)_bindlessBuffers[i].instanceLookupArray.bufferAddress
                + ((pCmd->firstInstance + pCmd->instanceCount)
                   * sizeof(unsigned int))
            );
            pCmd->instanceCount++;
            *pIndex = ret->instanceDataOffset
                      / sizeof(BindlessInstanceData
                      ); // offset divided by size to get index
        }
    }

    return ret;
}

void BindlessRenderSystem::DestroyComponent(
    BindlessRenderSystemComponent*& component
) {
    NEEDS_IMPLEMENTATION();
}

void BindlessRenderSystem::updateTextureDescriptorSet() {
    DEBUG("updating texture descirptor set");
    for (size_t i = 0; i < NUM_FRAME_IN_FLIGHT; i++) {
        std::array<VkWriteDescriptorSet, 1> descriptorWrites{};
        descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[0].dstSet = this->_descriptorSets[i];
        descriptorWrites[0].dstBinding = (int)BindingLocation::TEXTURE_SAMPLER;
        descriptorWrites[0].dstArrayElement = 0;
        descriptorWrites[0].descriptorType
            = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        descriptorWrites[0].descriptorCount
            = _textureDescriptorInfoIdx; // descriptors are 0-indexed, +1 for
                                         // the # of valid samplers
        descriptorWrites[0].pImageInfo = _textureDescriptorInfo.data();

        DEBUG("descriptor count: {}", descriptorWrites[0].descriptorCount);

        vkUpdateDescriptorSets(
            _device->logicalDevice,
            descriptorWrites.size(),
            descriptorWrites.data(),
            0,
            nullptr
        );
    }
};

void BindlessRenderSystem::FlagUpdate(Entity* entity) {
    ASSERT(entity->GetComponent<BindlessRenderSystemComponent>() != nullptr);
    // internally we push the entity to the update queue for each buffer, so
    // that it can be updated before the buffer are used for drawing
    // TODO: can have another staging buffer layer for better flushing
    // performance
    for (int i = 0; i < _bufferUpdateQueue.size(); i++) {
        _bufferUpdateQueue[i].push_back(entity);
    }
}

BindlessRenderSystem::RenderBatch BindlessRenderSystem::createRenderBatch(
    const std::string& meshPath,
    unsigned int batchSize
) {
    // FIXME: only the first render batch of a model needs to re-load model,
    // the rest should be reusing the first batch's vertex & index buffers
    DEBUG("creating render batch for {}", meshPath);
    DeletionStack del;

    DEBUG("Load vertex & index buffers");
    // load mesh into vertex and index buffer
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
    CoreUtils::loadModel(meshPath.c_str(), vertices, indices);

    VkDeviceSize vertexBufferSize = sizeof(Vertex) * vertices.size();
    VkDeviceSize indexBufferSize = sizeof(uint32_t) * indices.size();

    // make staging buffer for both vertex and index buffer
    std::pair<VkBuffer, VkDeviceMemory> res
        = CoreUtils::createVulkanStagingBuffer(
            _device->physicalDevice,
            _device->logicalDevice,
            vertexBufferSize + indexBufferSize
        );
    VkBuffer stagingBuffer = res.first;
    VkDeviceMemory stagingBufferMemory = res.second;
    del.push([this, stagingBuffer, stagingBufferMemory]() {
        vkDestroyBuffer(_device->logicalDevice, stagingBuffer, nullptr);
        vkFreeMemory(_device->logicalDevice, stagingBufferMemory, nullptr);
    });

    // copy vertex and index buffer to staging buffer
    void* vertexBufferDst;
    void* indexBufferDst;
    vkMapMemory(
        _device->logicalDevice,
        stagingBufferMemory,
        0,
        vertexBufferSize,
        0,
        &vertexBufferDst
    );
    memcpy(vertexBufferDst, vertices.data(), (size_t)vertexBufferSize);
    vkUnmapMemory(_device->logicalDevice, stagingBufferMemory);

    vkMapMemory(
        _device->logicalDevice,
        stagingBufferMemory,
        vertexBufferSize,
        indexBufferSize,
        0,
        &indexBufferDst
    );
    memcpy(indexBufferDst, indices.data(), (size_t)indexBufferSize);
    vkUnmapMemory(_device->logicalDevice, stagingBufferMemory);

    // copy from staging buffer to vertex/index buffer array
    CoreUtils::copyVulkanBuffer(
        _device->logicalDevice,
        _device->graphicsCommandPool,
        _device->graphicsQueue,
        stagingBuffer,
        _vertexBuffers.buffer,
        vertexBufferSize,
        0,
        _vertexBuffersWriteOffset
    );

    CoreUtils::copyVulkanBuffer(
        _device->logicalDevice,
        _device->graphicsCommandPool,
        _device->graphicsQueue,
        stagingBuffer,
        _indexBuffers.buffer,
        indexBufferSize,
        vertexBufferSize, // skip the vertex buffer region in staging buffer
        _indexBuffersWriteOffset
    );

    // create a draw command and store into `drawCommandArray`
    VkDrawIndexedIndirectCommand cmd{};
    cmd.firstIndex = _indexBuffersWriteOffset / sizeof(unsigned int);
    cmd.indexCount = indices.size();
    cmd.vertexOffset = _vertexBuffersWriteOffset / sizeof(Vertex);
    cmd.instanceCount = 0; // draw 0 instance by default
    cmd.firstInstance = _instanceLookupArrayOffset / sizeof(unsigned int);

    for (int i = 0; i < NUM_FRAME_IN_FLIGHT; i++) {
        // copy draw command to this addr
        // NOTE: here we assume blindless buffer is CPU coherent and
        // accessible.
        // TODO: use staging for draw command creation; after creation draw
        // command is almost never read/written by the CPU
        void* addr = (char*)_bindlessBuffers[i].drawCommandArray.bufferAddress
                     + _drawCommandArrayOffset;

        memcpy(addr, std::addressof(cmd), sizeof(cmd));
        // NOTE: we only handle creation of drawCommand, but not deletion so
        // far
    }

    RenderBatch batch{
        .maxSize = batchSize,
        .instanceCount = 0,
        .drawCmdOffset = _drawCommandArrayOffset
    };

    // bump offsets

    // added a new draw command
    _drawCommandArrayOffset += sizeof(VkDrawIndexedIndirectCommand);

    // reserve `instanceNumber` * sizeof(unsigned int) in
    // instanceLookupArray
    // bumping the offset will do so
    _instanceLookupArrayOffset += batchSize * sizeof(unsigned int);

    // bump offset for next writes
    _vertexBuffersWriteOffset += vertexBufferSize;
    _indexBuffersWriteOffset += indexBufferSize;

    del.flush();

    return batch;
}

void BindlessRenderSystem::createBindlessResources() {
    for (int i = 0; i < NUM_FRAME_IN_FLIGHT; i++) {
        _device->CreateBufferInPlace(
            5000,
            VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
                | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            _bindlessBuffers[i].drawCommandArray
        );
        _device->CreateBufferInPlace(
            5000,
            VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
                | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            _bindlessBuffers[i].instanceDataArray
        );
        _device->CreateBufferInPlace(
            5000,
            VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
                | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            _bindlessBuffers[i].instanceLookupArray
        );
        _deletionStack.push([this]() {
            for (int i = 0; i < NUM_FRAME_IN_FLIGHT; i++) {
                _bindlessBuffers[i].instanceLookupArray.Cleanup();
                _bindlessBuffers[i].drawCommandArray.Cleanup();
                _bindlessBuffers[i].instanceDataArray.Cleanup();
            }
        });
    }

    // FIXME: the memory should have DEVICE_LOCAL_BIT
    // allocate large vertex and index buffer
    _device->CreateBufferInPlace(
        500000,
        VK_BUFFER_USAGE_TRANSFER_DST_BIT // can be used as destination in a
                                         // memory transfer operation
            | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
            | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, // local to the GPU for
                                                    // faster
        _vertexBuffers
    );
    _device->CreateBufferInPlace(
        500000,
        VK_BUFFER_USAGE_TRANSFER_DST_BIT // can be used as destination in a
                                         // memory transfer operation
            | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
            | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, // local to the GPU for
                                                    // faster
        _indexBuffers
    );
}
