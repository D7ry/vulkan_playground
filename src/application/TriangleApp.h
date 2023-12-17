#pragma once
#include "VulkanApplication.h"

class TriangleApp : public VulkanApplication {
      protected:
      virtual void createGraphicsPipeline() override;
      virtual void createRenderPass() override;
      virtual void createFramebuffers() override;
      virtual void createCommandPool() override;
      virtual void createCommandBuffer() override;
      virtual void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex) override;

      virtual void drawFrame() override;
};