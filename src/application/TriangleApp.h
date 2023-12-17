#pragma once
#include "VulkanApplication.h"
#include "structs/Vertex.h"

class TriangleApp : public VulkanApplication {
      protected:
      virtual void createGraphicsPipeline() override;
      virtual void createRenderPass() override;
      virtual void createFramebuffers() override;
      virtual void createCommandPool() override;
      virtual void createCommandBuffer() override;
      virtual void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex) override;
      virtual void createVertexBuffer() override;

      virtual void drawFrame() override;

      // Vertices, for demonstration purposes
      const std::vector<Vertex> _vertices = {
      {{0.0f, -0.5f}, {1.0f, 0.0f, 0.0f}},
      {{0.5f, 0.5f}, {0.0f, 1.0f, 0.0f}},
      {{-0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}}
      };
};