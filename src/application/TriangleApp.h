#pragma once
#include "VulkanApplication.h"

class TriangleApp : public VulkanApplication {
      protected:
      virtual void createGraphicsPipeline() override;
      virtual void createRenderPass() override;
};