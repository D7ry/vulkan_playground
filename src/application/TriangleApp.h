#pragma once
#include "VulkanApplication.h"
#include "components/Camera.h"
#include "components/ImGuiManager.h"
#include "components/TextureManager.h"
#include "structs/Vertex.h"
#include <cstdint>
#include <memory>

class TriangleApp : public VulkanApplication {
      protected:
        virtual void createRenderPass() override;
        virtual void createFramebuffers() override;
        //virtual void createCommandPool() override;
        //virtual void createCommandBuffer() override;
        virtual void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex) override;
        virtual void createDepthBuffer() override;
        virtual void drawFrame() override;
        virtual void updateUniformBufferData(uint32_t frameIndex) override;

        //virtual void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) override;
        virtual void cursorPosCallback(GLFWwindow* window, double xpos, double ypos) override;
        virtual void renderImGui() override;
        virtual void middleInit() override;
        virtual void postInit() override;

        virtual void preCleanup() override;

};