#pragma once
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_vulkan.h"
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

#include "VulkanUtils.h"

class ImGuiManager {
      public:
        void InitializeImgui();

        void BindVulkanResources(
                GLFWwindow* window,
                VkInstance instance,
                VkPhysicalDevice physicalDevice,
                VkDevice device,
                uint32_t graphicsQueueFamilyIndex,
                VkQueue graphicsQueue,
                int imageCount
        );

        void InitializeRenderPass(VkDevice logicalDevice, VkFormat swapChainImageFormat);

        void InitializeFonts();

        void InitializeCommandPoolAndBuffers(int bufferCount, VkDevice device, uint32_t queueFamilyIndex);

        void DestroyFrameBuffers(VkDevice device);

        void InitializeFrameBuffer(int bufferCount, VkDevice device, std::vector<VkImageView>& swapChainImageViews, VkExtent2D extent);

        void InitializeDescriptorPool(int frames_in_flight, VkDevice logicalDevice);

        void RenderFrame();

        void RecordCommandBuffer(int currentFrameInFlight, int imageIndex, VkExtent2D swapChainExtent);

        VkCommandBuffer GetCommandBuffer(uint32_t currentFrame);

      private:
        VkRenderPass _imGuiRenderPass; // render pass sepcifically for imgui
        VkDescriptorPool _imguiDescriptorPool;
        std::vector<VkCommandBuffer> _imGuiCommandBuffers;
        VkCommandPool _imGuiCommandPool;
        std::vector<VkFramebuffer> _imGuiFramebuffers;
        VkClearValue _imguiClearValue = {0.0f, 0.0f, 0.0f, 0.0f}; // transparent, unused
};