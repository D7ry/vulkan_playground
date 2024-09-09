#pragma once
#include "glm/glm.hpp"
#include <vulkan/vulkan_core.h>

#include "lib/VQBuffer.h"
class Camera;

struct GraphicsTickData
{
    int currentFrameInFlight; // used to index into command buffer of render
                              // systems
    int currentSwapchainImageIndex;
    VkFramebuffer
        currentFB; // result of using `vkAcquireNextImageKHR` on engine
                   // swapchain, to obtain the currently active fb
    VkCommandBuffer
        currentCB; // a active command buffer.
                   // vkBeginCommandBuffer should've been called on it
    VkExtent2D currentFBextend;
    glm::mat4 mainProjectionMatrix;
};

class Profiler;

struct TickData
{
    const Camera* mainCamera;
    // std::vector<VkFramebuffer>* swapChainFB;
    double deltaTime;
    GraphicsTickData graphics;
    Profiler* profiler;
};

class VQDevice;
class TextureManager;

struct InitData
{
    VQDevice* device;
    VkFormat swapChainImageFormat;
    TextureManager* textureManager;

    struct
    {
        VkRenderPass mainPass;
    } renderPass;

    /**
     * points to initialized buffer of static engine ubo
     * the callee can bind static ubo to its descriptor set by using:
     *
     * InitData initData->engineUBOStatic[i]
     *
     */
    std::array<VkDescriptorBufferInfo, NUM_FRAME_IN_FLIGHT>
        engineUBOStaticDescriptorBufferInfo;
};
