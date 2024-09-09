#pragma once
#include "glm/glm.hpp"
#include <vulkan/vulkan_core.h>

#include "lib/VQBuffer.h"
class Camera;

// static ubo that gets updated by the engine every frame
struct EngineUBOStatic
{
    glm::mat4 view;              // view matrix
    glm::mat4 proj;              // proj matrix
    float timeSinceStartSeconds; // time (seconds) since engine start; may use
                                 // it for some interesting interpolation
};

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
