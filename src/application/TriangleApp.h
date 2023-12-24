#pragma once
#include "VulkanApplication.h"
#include "components/Camera.h"
#include "components/ImGuiManager.h"
#include "components/TextureManager.h"
#include "structs/Vertex.h"
#include <memory>

class TriangleApp : public VulkanApplication {
      protected:
        virtual void createGraphicsPipeline() override;
        virtual void createRenderPass() override;
        virtual void createFramebuffers() override;
        virtual void createCommandPool() override;
        virtual void createCommandBuffer() override;
        virtual void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex) override;
        virtual void createVertexBuffer() override;
        virtual void createIndexBuffer() override;
        virtual void createDepthBuffer() override;
        virtual void createUniformBuffers() override;
        virtual void createDescriptorSetLayout() override;
        virtual void createDescriptorPool() override;
        virtual void createDescriptorSets() override;
        virtual void drawFrame() override;
        virtual void updateUniformBufferData(uint32_t frameIndex) override;

        virtual void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) override;
        virtual void renderImGui() override;
        virtual void middleInit() override;
        virtual void postInit() override;

        virtual void preCleanup() override;

        // Vertices, for demonstration purposes
        const std::vector<Vertex> _vertices
                = {{{-0.5f, -0.5f, 0.0f}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}},
                   {{0.5f, -0.5f, 0.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}},
                   {{0.5f, 0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}},
                   {{-0.5f, 0.5f, 0.0f}, {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f}},

                   {{-0.5f, -0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
                   {{0.5f, -0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}},
                   {{0.5f, 0.5f, -0.5f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},
                   {{-0.5f, 0.5f, -0.5f}, {1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}}};

        const std::vector<uint16_t> _indices = {0, 1, 2, 2, 3, 0, 4, 5, 6, 6, 7, 4};

        Camera _camera = Camera(glm::vec3(-2.0f, 0.0f, 2.0f), glm::vec3(-45, 0, 0));
        std::unique_ptr<TextureManager> _textureManager = nullptr;
};