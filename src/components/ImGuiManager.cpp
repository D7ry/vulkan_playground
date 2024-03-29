#include "ImGuiManager.h"
#include "imgui.h"
#include "imgui_impl_glfw.h"

void ImGuiManager::InitializeImgui() {
    INFO("Initializing imgui...");
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;  // Enable Gamepad Controls

    ImGui::StyleColorsDark();
    INFO("ImGui initialized.");
}

void ImGuiManager::BindVulkanResources(
    GLFWwindow* window,
    VkInstance instance,
    VkPhysicalDevice physicalDevice,
    VkDevice device,
    uint32_t graphicsQueueFamilyIndex,
    VkQueue graphicsQueue,
    int imageCount
) {
    if (this->_imGuiRenderPass == VK_NULL_HANDLE) {
        FATAL("Render pass must be initialized before binding vulkan resources!");
    }
    ImGui_ImplGlfw_InitForVulkan(window, true);

    ImGui_ImplVulkan_InitInfo initInfo = {};
    initInfo.Instance = instance;
    initInfo.PhysicalDevice = physicalDevice;
    initInfo.Device = device;
    initInfo.QueueFamily = graphicsQueueFamilyIndex;
    initInfo.Queue = graphicsQueue;
    initInfo.PipelineCache = VK_NULL_HANDLE;        // keeping it none is fine
    initInfo.DescriptorPool = _imguiDescriptorPool; // imgui custom descriptor pool
    initInfo.Allocator = VK_NULL_HANDLE;            // keeping it none is fine
    initInfo.MinImageCount = 2;
    initInfo.ImageCount = imageCount;
    initInfo.CheckVkResultFn = nullptr;
    ImGui_ImplVulkan_Init(&initInfo, _imGuiRenderPass);
};

void ImGuiManager::InitializeRenderPass(VkDevice logicalDevice, VkFormat swapChainImageFormat) {
    INFO("Creating render pass...");
    VkAttachmentDescription colorAttachment{};
    colorAttachment.format = swapChainImageFormat;
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;

    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD; // load from previou pass
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;

    // don't care about stencil
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
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
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    VkRenderPassCreateInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = 1;
    renderPassInfo.pAttachments = &colorAttachment;
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies = &dependency;

    if (vkCreateRenderPass(logicalDevice, &renderPassInfo, nullptr, &this->_imGuiRenderPass) != VK_SUCCESS) {
        FATAL("Failed to create render pass!");
    }
}

void ImGuiManager::InitializeFonts() { ImGui_ImplVulkan_CreateFontsTexture(); }

void ImGuiManager::InitializeCommandPoolAndBuffers(int bufferCount, VkDevice device, uint32_t queueFamilyIndex) {
    VulkanUtils::createCommandPool(
        &_imGuiCommandPool, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT, queueFamilyIndex, device
    );
    VulkanUtils::createCommandBuffers(_imGuiCommandBuffers, bufferCount, _imGuiCommandPool, device);
}

void ImGuiManager::DestroyFrameBuffers(VkDevice device) {
    INFO("Destroying imgui frame buffers...");
    for (auto framebuffer : _imGuiFramebuffers) {
        vkDestroyFramebuffer(device, framebuffer, nullptr);
    }
}

void ImGuiManager::InitializeFrameBuffer(
    int bufferCount,
    VkDevice device,
    std::vector<VkImageView>& swapChainImageViews,
    VkExtent2D extent
) {
    INFO("Creating imgui frame buffers...");
    if (swapChainImageViews.size() != bufferCount) {
        FATAL("Swap chain image views must be the same size as the number of buffers!");
    }
    if (this->_imGuiRenderPass == VK_NULL_HANDLE) {
        FATAL("Render pass must be initialized before creating frame buffers!");
    }
    _imGuiFramebuffers.resize(bufferCount);
    VkImageView attachment[1];
    VkFramebufferCreateInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    info.renderPass = _imGuiRenderPass;
    info.attachmentCount = 1;
    info.pAttachments = attachment;
    info.width = extent.width;
    info.height = extent.height;
    info.layers = 1;
    for (uint32_t i = 0; i < bufferCount; i++) {
        attachment[0] = swapChainImageViews[i];
        vkCreateFramebuffer(device, &info, nullptr, &_imGuiFramebuffers[i]);
    }
    INFO("Imgui frame buffers created.");
}

void ImGuiManager::InitializeDescriptorPool(int frames_in_flight, VkDevice logicalDevice) {
    // create a pool that will allocate to actual descriptor sets
    uint32_t descriptorSetCount = static_cast<uint32_t>(frames_in_flight);
    VkDescriptorPoolSize poolSizes[] = {
        {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, descriptorSetCount} // image sampler for imgui
    };

    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = sizeof(poolSizes) / sizeof(VkDescriptorPoolSize); // number of pool sizes
    poolInfo.pPoolSizes = poolSizes;
    poolInfo.maxSets = descriptorSetCount; // number of descriptor sets, set to the number of frames in flight
    poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;

    if (vkCreateDescriptorPool(logicalDevice, &poolInfo, nullptr, &_imguiDescriptorPool) != VK_SUCCESS) {
        FATAL("Failed to create descriptor pool!");
    }
}

void ImGuiManager::RenderFrame() {
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();

    ImGui::NewFrame();
    if (this->_imguiRenderCallback != nullptr) {
        this->_imguiRenderCallback();
    } else {
        ImGui::ShowDemoWindow();
    }
    ImGui::Render();
}

void ImGuiManager::RecordCommandBuffer(int currentFrameInFlight, int imageIndex, VkExtent2D swapChainExtent) {
    VkCommandBuffer commandBuffer = _imGuiCommandBuffers[currentFrameInFlight];
    vkResetCommandBuffer(commandBuffer, 0); // clear off old commands

    // start recording commands
    VkCommandBufferBeginInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    info.flags |= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    if (vkBeginCommandBuffer(commandBuffer, &info) != VK_SUCCESS) {
        FATAL("Failed to begin recording command buffer!");
    }

    // start render pass
    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = _imGuiRenderPass;
    renderPassInfo.framebuffer = _imGuiFramebuffers[imageIndex];
    renderPassInfo.renderArea.extent = swapChainExtent;
    renderPassInfo.renderArea.offset = {0, 0};
    renderPassInfo.clearValueCount = 0;
    renderPassInfo.pClearValues = nullptr;

    vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

    ImDrawData* drawData = ImGui::GetDrawData();
    if (drawData == nullptr) {
        FATAL("Draw data is null!");
    }
    ImGui_ImplVulkan_RenderDrawData(drawData, commandBuffer);

    vkCmdEndRenderPass(commandBuffer);
    vkEndCommandBuffer(commandBuffer);
}

VkCommandBuffer ImGuiManager::GetCommandBuffer(uint32_t currentFrame) { return _imGuiCommandBuffers[currentFrame]; }

void ImGuiManager::Cleanup(VkDevice logicalDevice) {
    INFO("Cleaning up imgui...");
    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    vkDestroyRenderPass(logicalDevice, _imGuiRenderPass, nullptr);
    vkDestroyDescriptorPool(logicalDevice, _imguiDescriptorPool, nullptr);
    vkDestroyCommandPool(logicalDevice, _imGuiCommandPool, nullptr);
}

void ImGuiManager::BindRenderCallback(std::function<void(void)> callback) { this->_imguiRenderCallback = callback; }
