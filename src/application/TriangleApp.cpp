#include <glm/fwd.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "TriangleApp.h"
#include "components/InputManager.h"
#include "components/TextureManager.h"
#include "imgui.h"
#include "structs/UniformBuffer.h"
#include "components/Animation.h"
#include "components/ShaderUtils.h"
#include <GLFW/glfw3.h>
#include <cstdint>
#include <glm/trigonometric.hpp>
#include <vulkan/vulkan_core.h>
#include "components/VulkanUtils.h"

// for testing only
#include "components/MetaPipeline.h"

#include "components/MeshRenderManager.h"
#include "components/MeshRenderer.h"

static MeshRenderManager renderManager;

const std::string SAMPLE_TEXTURE_PATH = "../textures/viking_room.png";
const std::string SAMPLE_MESH_PATH = "../meshes/viking_room.obj";
MeshRenderer* render = new MeshRenderer(SAMPLE_MESH_PATH.data(), SAMPLE_TEXTURE_PATH.data());
MeshRenderer* render2 = new MeshRenderer(SAMPLE_MESH_PATH.data(), SAMPLE_TEXTURE_PATH.data());
MeshRenderer* render3 = new MeshRenderer("../meshes/spot.obj", "../textures/spot.png");
MeshRenderer* render4 = new MeshRenderer("../meshes/smoking_room.obj", "../textures/smoking_room.png");



static bool updatedCursor = false;
static int prevX = -1;
static int prevY = -1;
void TriangleApp::cursorPosCallback(GLFWwindow* window, double xpos, double ypos) {
        if (!updatedCursor) {
                prevX = xpos;
                prevY = ypos;
                updatedCursor = true;
        }
        double deltaX = prevX - xpos;
        double deltaY = prevY - ypos;
        // handle camera movement
        deltaX *= 0.3;
        deltaY *= 0.3; // make movement slower
        if (viewMode) {
                _camera.ModRotation( deltaX, deltaY, 0);
        }
        prevX = xpos;
        prevY = ypos;
}

void TriangleApp::renderImGui() {
        if (ImGui::Begin("Debug")) {
                ImGui::SetWindowPos(ImVec2(0, 0), ImGuiCond_Once);
                ImGui::SetWindowSize(ImVec2(400, 400), ImGuiCond_Once);
                ImGui::Text("Framerate: %f", ImGui::GetIO().Framerate);
                ImGui::Separator();
                ImGui::Text("Camera");
                if (ImGui::BeginChild("Camera")) {
                        ImGui::Text(
                                "Position: (%f, %f, %f)",
                                _camera.GetPosition().x,
                                _camera.GetPosition().y,
                                _camera.GetPosition().z
                        );
                        ImGui::Text(
                                "Yaw: %f Pitch: %f Roll: %f",
                                _camera.GetRotation().y,
                                _camera.GetRotation().x,
                                _camera.GetRotation().z
                        );
                        if (viewMode) {
                                ImGui::Text("View Mode: Active");
                        } else {
                                ImGui::Text("View Mode: Deactive");
                        }
                        ImGui::EndChild();
                }
        }
        ImGui::End();
}

void TriangleApp::middleInit() {
        TextureManager::GetSingleton()->Init(_device); // pass device to texture manager for it to start loading
        TextureManager::GetSingleton()->LoadTexture(SAMPLE_TEXTURE_PATH);
}

void TriangleApp::postInit() {

        render->RegisterRenderManager(&renderManager);

        render2->RegisterRenderManager(&renderManager);
        render2->transform.position = glm::vec3(0, 0, 2);

        render3->RegisterRenderManager(&renderManager);
        render3->transform.position = glm::vec3(0, 2, 2);

        render3->RegisterRenderManager(&renderManager);
        render3->transform.position = glm::vec3(0, 2, 2);

        render4->RegisterRenderManager(&renderManager);
        render4->transform.position = glm::vec3(0, 0, -10);
        render4->transform.rotation = {90, 0, 0};

        renderManager.PrepareRendering(MAX_FRAMES_IN_FLIGHT, _renderPass, _device);

        //_inputManager->RegisterCallback(GLFW_KEY_UP, InputManager::KeyCallbackCondition::HOLD, []() {render4->Rotate(10, 0, 0);});
}


void TriangleApp::createRenderPass() {
        INFO("Creating render pass...");
        VkAttachmentDescription colorAttachment{};
        colorAttachment.format = this->_swapChainImageFormat;
        colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;

        colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;

        // don't care about stencil
        colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

        colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        //colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        colorAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL; // for imgui

        VkAttachmentReference colorAttachmentRef{};
        colorAttachmentRef.attachment = 0;
        colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        VkAttachmentDescription depthAttachment{};
        depthAttachment.format = VulkanUtils::findDepthFormat(_device->physicalDevice);
        depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
        depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        VkAttachmentReference depthAttachmentRef{};
        depthAttachmentRef.attachment = 1;
        depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        // set up subpass

        VkSubpassDescription subpass{};
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount = 1;
        subpass.pColorAttachments = &colorAttachmentRef;
        subpass.pDepthStencilAttachment = &depthAttachmentRef;

        // dependency to make sure that the render pass waits for the image to be available before drawing
        VkSubpassDependency dependency{};
        dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
        dependency.dstSubpass = 0;
        dependency.srcStageMask
                = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        dependency.srcAccessMask = 0;
        dependency.dstStageMask
                = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

        std::array<VkAttachmentDescription, 2> attachments = {colorAttachment, depthAttachment};

        VkRenderPassCreateInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
        renderPassInfo.pAttachments = attachments.data();
        renderPassInfo.subpassCount = 1;
        renderPassInfo.pSubpasses = &subpass;
        renderPassInfo.dependencyCount = 1;
        renderPassInfo.pDependencies = &dependency;

        if (vkCreateRenderPass(_device->logicalDevice, &renderPassInfo, nullptr, &this->_renderPass) != VK_SUCCESS) {
                FATAL("Failed to create render pass!");
        }
}
void TriangleApp::createFramebuffers() {
        INFO("Creating {} framebuffers...", this->_swapChainImageViews.size());
        this->_swapChainFrameBuffers.resize(this->_swapChainImageViews.size());
        // iterate through image views and create framebuffers
        if (_renderPass == VK_NULL_HANDLE) {
                FATAL("Render pass is null!");
        }
        for (size_t i = 0; i < _swapChainImageViews.size(); i++) {
                VkImageView attachments[] = {_swapChainImageViews[i], _depthImageView};
                VkFramebufferCreateInfo framebufferInfo{};
                framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
                framebufferInfo.renderPass = _renderPass; // each framebuffer is associated with a render pass;  they need to be compatible i.e.
                                                          // having same number of attachments and same formats
                framebufferInfo.attachmentCount = sizeof(attachments) / sizeof(VkImageView); 
                framebufferInfo.pAttachments = attachments;
                framebufferInfo.width = _swapChainExtent.width;
                framebufferInfo.height = _swapChainExtent.height;
                framebufferInfo.layers = 1; // number of layers in image arrays
                if (vkCreateFramebuffer(_device->logicalDevice, &framebufferInfo, nullptr, &_swapChainFrameBuffers[i]) != VK_SUCCESS) {
                        FATAL("Failed to create framebuffer!");
                }
        }
        _imguiManager.InitializeFrameBuffer(this->_swapChainImageViews.size(), _device->logicalDevice, _swapChainImageViews, _swapChainExtent);
        INFO("Framebuffers created.");
}

// void TriangleApp::createCommandPool() {
//         INFO("Creating command pool...");
//         QueueFamilyIndices queueFamilyIndices = findQueueFamilies(this->_physicalDevice);

//         VkCommandPoolCreateInfo poolInfo{};
//         poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
//         poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT; // ALlow command buffers to be re-recorded individually
//         // we want to re-record the command buffer every single frame.
//         poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();

//         if (vkCreateCommandPool(this->_logicalDevice, &poolInfo, nullptr, &this->_commandPool) != VK_SUCCESS) {
//                 FATAL("Failed to create command pool!");
//         }
//         INFO("Command pool created.");
// }

// void TriangleApp::createCommandBuffer() {
//         INFO("Creating command buffer");

//         this->_commandBuffers.resize(this->MAX_FRAMES_IN_FLIGHT);

//         VkCommandBufferAllocateInfo allocInfo{};
//         allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
//         allocInfo.commandPool = this->_commandPool;
//         allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
//         allocInfo.commandBufferCount = (uint32_t)this->_commandBuffers.size();

//         if (vkAllocateCommandBuffers(this->_logicalDevice, &allocInfo, this->_commandBuffers.data()) != VK_SUCCESS) {
//                 FATAL("Failed to allocate command buffers!");
//         }
// }

void TriangleApp::recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex) { // called every frame
        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = 0;                  // Optional
        beginInfo.pInheritanceInfo = nullptr; // Optional

        if (vkBeginCommandBuffer(this->_commandBuffers[_currentFrame], &beginInfo) != VK_SUCCESS) {
                FATAL("Failed to begin recording command buffer!");
        }

        // start render pass
        VkRenderPassBeginInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassInfo.renderPass = this->_renderPass;
        renderPassInfo.framebuffer = this->_swapChainFrameBuffers[imageIndex];

        renderPassInfo.renderArea.offset = {0, 0};
        renderPassInfo.renderArea.extent = this->_swapChainExtent;

        std::array<VkClearValue, 2> clearValues{};
        clearValues[0].color = {{0.0f, 0.0f, 0.0f, 1.0f}};
        clearValues[1].depthStencil = {1.0f, 0};

        renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
        renderPassInfo.pClearValues = clearValues.data();

        vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

        // TODO: this doesn't need to update every frame
        VkViewport viewport{};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = static_cast<float>(_swapChainExtent.width);
        viewport.height = static_cast<float>(_swapChainExtent.height);
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
        vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

        VkRect2D scissor{};
        scissor.offset = {0, 0};
        scissor.extent = _swapChainExtent;
        vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

        renderManager.RecordRenderCommands(commandBuffer, _currentFrame);
        vkCmdEndRenderPass(commandBuffer);

        if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
                FATAL("Failed to record command buffer!");
        }
}

void TriangleApp::createDepthBuffer() {
        INFO("Creating depth buffer...");
        VkFormat depthFormat = VulkanUtils::findBestFormat(
                {VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT},
                VK_IMAGE_TILING_OPTIMAL,
                VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT,
                _device->physicalDevice
        );
        VulkanUtils::createImage(
                _swapChainExtent.width,
                _swapChainExtent.height,
                depthFormat,
                VK_IMAGE_TILING_OPTIMAL,
                VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                _depthImage,
                _depthImageMemory,
                _device->physicalDevice,
                _device->logicalDevice
        );
        _depthImageView = VulkanUtils::createImageView(_depthImage, _device->logicalDevice, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT);
}

void TriangleApp::updateUniformBufferData(uint32_t frameIndex) {
        float deltaTime = _deltaTimer.GetDeltaTime();
        render->Rotate(90 *deltaTime, 0, 0);
        render2->Rotate(0, 90 * deltaTime, 0);
        render3->Rotate(0, 0, 90 * deltaTime);
        renderManager.UpdateUniformBuffers(frameIndex, this->_camera.GetViewMatrix(), glm::perspective(glm::radians(90.f), _swapChainExtent.width / (float)_swapChainExtent.height, 0.1f, 100.f));
        // static Animation::StopWatchSeconds timer;
        // float time = timer.elapsed();
        // UniformBuffer ubo{};
        // auto initialPos = glm::mat4(1.f);
        // ubo.model = initialPos;
        // ubo.model = glm::rotate(ubo.model, time * glm::radians(15.f), glm::vec3(0.f, 0.f, 1.f));
        // // ubo.model = glm::translate(ubo.model, glm::vec3(0.f, 0.f, time - int(time)));
        // ubo.view = this->_camera.GetViewMatrix();
        // // ubo.view = glm::lookAt(glm::vec3(2.f, 2.f, 2.f), glm::vec3(0.f, 0.f, 0.f), glm::vec3(0.f, 0.f, 1.f));
        // ubo.proj = glm::perspective(glm::radians(90.f), _swapChainExtent.width / (float)_swapChainExtent.height, 0.1f, 100.f);

        // ubo.proj[1][1] *= -1; // flip y coordinate
        // // TODO: use push constants for small data update
        // memcpy(_uniformBuffersData[frameIndex], &ubo, sizeof(ubo));
}

void TriangleApp::drawFrame() {
        //  Wait for the previous frame to finish
        vkWaitForFences(_device->logicalDevice, 1, &this->_fenceInFlight[this->_currentFrame], VK_TRUE, UINT64_MAX);

        //  Acquire an image from the swap chain
        uint32_t imageIndex;
        VkResult result = vkAcquireNextImageKHR(
                this->_device->logicalDevice, _swapChain, UINT64_MAX, _semaImageAvailable[this->_currentFrame], VK_NULL_HANDLE, &imageIndex
        );
        if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
                this->recreateSwapChain();
                return;
        } else if (result != VK_SUCCESS) {
                FATAL("Failed to acquire swap chain image!");
        }

        // lock the fence
        vkResetFences(this->_device->logicalDevice, 1, &this->_fenceInFlight[this->_currentFrame]);

        //  Record a command buffer which draws the scene onto that image
        vkResetCommandBuffer(this->_commandBuffers[this->_currentFrame], 0);
        this->recordCommandBuffer(this->_commandBuffers[this->_currentFrame], imageIndex);
        _imguiManager.RecordCommandBuffer(this->_currentFrame, imageIndex, _swapChainExtent);

        this->updateUniformBufferData(_currentFrame);

        //  Submit the recorded command buffer
        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

        VkSemaphore waitSemaphores[]
                = {_semaImageAvailable[_currentFrame]}; // use imageAvailable semaphore to make sure that the image is available before drawing
        VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
        std::array<VkCommandBuffer, 2> submitCommandBuffers = {_commandBuffers[_currentFrame], _imguiManager.GetCommandBuffer(_currentFrame)};

        submitInfo.waitSemaphoreCount = 1;
        submitInfo.pWaitSemaphores = waitSemaphores;
        submitInfo.pWaitDstStageMask = waitStages;
        submitInfo.commandBufferCount = static_cast<uint32_t>(submitCommandBuffers.size());
        submitInfo.pCommandBuffers = submitCommandBuffers.data();

        VkSemaphore signalSemaphores[] = {_semaRenderFinished[_currentFrame]};
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = signalSemaphores;

        // the submission does not start until vkAcquireNextImageKHR returns, and downs the corresponding _semaRenderFinished semapohre once it's done.
        if (vkQueueSubmit(_device->graphicsQueue, 1, &submitInfo, _fenceInFlight[_currentFrame]) != VK_SUCCESS) {
                FATAL("Failed to submit draw command buffer!");
        }

        //  Present the swap chain image
        VkPresentInfoKHR presentInfo{};
        presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

        // set up semaphore, so that  after submitting to the queue, we wait  for the 
        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores = signalSemaphores; // wait for render to finish before presenting

        VkSwapchainKHR swapChains[] = {_swapChain};
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = swapChains;
        presentInfo.pImageIndices = &imageIndex; // specify which image to present
        presentInfo.pResults = nullptr;          // Optional: can be used to check if presentation was successful

        // the present doesn't happen until the render is finished, and the semaphore is signaled(result of vkQueueSubimt)
        result = vkQueuePresentKHR(_device->presentationQueue, &presentInfo);
        if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || this->_framebufferResized) {
                this->recreateSwapChain();
                this->_framebufferResized = false;
        } else if (result != VK_SUCCESS) {
                FATAL("Failed to present swap chain image!");
        }
        //  Advance to the next frame
        _currentFrame = (_currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

void TriangleApp::preCleanup() {
        TextureManager::GetSingleton()->Cleanup();
        //metapipeline.Cleanup(_logicalDevice);
}