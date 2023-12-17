#include "TriangleApp.h"
#include "utils/ShaderUtils.h"
#include <vulkan/vulkan_core.h>

void TriangleApp::createGraphicsPipeline() {
        INFO("Creating TriangleAPP Graphics Pipeline...");

        INFO("setting up shader modules...");
        // programmable stages
        VkShaderModule vertShaderModule = ShaderCreation::createShaderModule(this->_logicalDevice, "../shaders/frag_test.frag.spv");
        VkShaderModule fragShaderModule = ShaderCreation::createShaderModule(this->_logicalDevice, "../shaders/vert_test.vert.spv");

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
        rasterizer.polygonMode = VK_POLYGON_MODE_FILL; // fill the area of the polygon with fragments
        rasterizer.lineWidth = 1.0f;                   // thickness of lines in terms of number of fragments
        rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;   // cull back faces
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
        multisampling.minSampleShading = 1.0f; // Optional
        multisampling.pSampleMask = nullptr; // Optional
        multisampling.alphaToCoverageEnable = VK_FALSE; // Optional
        multisampling.alphaToOneEnable = VK_FALSE; // Optional

        // depth buffering
        // we're not there yet, so we'll disable it for now

        // color blending - combining color from the fragment shader with color that is already in the framebuffer

        INFO("setting up color blending...");
        // controls configuration per attached frame buffer
        //https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkBlendOp.html
        //https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkBlendFactor.html
        VkPipelineColorBlendAttachmentState colorBlendAttachment{};
        colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        colorBlendAttachment.blendEnable = VK_FALSE; // set to true to enable blending
        colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
        colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
        colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD; // Optional
        colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
        colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
        colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD; // Optional

        // global color blending settings
        VkPipelineColorBlendStateCreateInfo colorBlending{};
        colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        colorBlending.logicOpEnable = VK_FALSE; // set to true to enable blending
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
        pipelineLayoutInfo.setLayoutCount = 0; // Optional
        pipelineLayoutInfo.pSetLayouts = nullptr; // Optional
        pipelineLayoutInfo.pushConstantRangeCount = 0; // Optional
        pipelineLayoutInfo.pPushConstantRanges = nullptr; // Optional

        if (vkCreatePipelineLayout(_logicalDevice, &pipelineLayoutInfo, nullptr, &this->_pipelineLayout) != VK_SUCCESS) {
                FATAL("Failed to create pipeline layout!");
        }


        vkDestroyShaderModule(_logicalDevice, fragShaderModule, nullptr);
        vkDestroyShaderModule(_logicalDevice, vertShaderModule, nullptr);
        INFO("Graphics pipeline created.");
}