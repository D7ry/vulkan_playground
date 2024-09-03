#pragma once
#include "glm/glm.hpp"
#include <vulkan/vulkan_core.h>

class Camera;

struct TickData
{
    const Camera* mainCamera;
    // std::vector<VkFramebuffer>* swapChainFB;
    double deltaTime;

    int currentFrameInFlight; // used to index into command buffer of render
                              // systems
    VkFramebuffer
        currentFB; // result of using `vkAcquireNextImageKHR` on engine
                   // swapchain, to obtain the currently active fb
    VkCommandBuffer currentCB; // a active command buffer.
                               // vkBeginCommandBuffer should've been called on it
    VkExtent2D currentFBextend;
    glm::mat4 mainProjectionMatrix;
};

class VQDevice;
class TextureManager;

struct InitData
{
    VQDevice* device;
    VkFormat swapChainImageFormat;
    TextureManager* textureManager;
};