#pragma once
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_vulkan.h"
#include "imgui.h"
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

#include "VulkanUtils.h"
#include "structs/TickData.h"

class ImGuiManager
{
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

    void InitializeRenderPass(
        VkDevice logicalDevice,
        VkFormat swapChainImageFormat
    );

    void InitializeFonts();

    void DestroyFrameBuffers(VkDevice device);

    void InitializeFrameBuffer(
        int bufferCount,
        VkDevice device,
        std::vector<VkImageView>& swapChainImageViews,
        VkExtent2D extent
    );

    void InitializeDescriptorPool(int frames_in_flight, VkDevice logicalDevice);

    // Create a new frame and records all ImGui::**** draws
    // for the draw calls to actually be presented, call RecordCommandBuffer(),
    // which pushes all draw calls to the CB.
    void BeginImGuiContext();

    // end the `BeginRecordImGui` context
    void EndImGuiContext();

    // Push all recorded ImGui UI elements onto the CB.
    void RecordCommandBuffer(const TickData* tickData);

    void Cleanup(VkDevice logicalDevice);

  private:
    VkRenderPass _imGuiRenderPass; // render pass sepcifically for imgui
    VkDescriptorPool _imguiDescriptorPool;
    std::vector<VkFramebuffer> _imGuiFramebuffers;
    VkClearValue _imguiClearValue
        = {0.0f, 0.0f, 0.0f, 0.0f}; // transparent, unused
};
