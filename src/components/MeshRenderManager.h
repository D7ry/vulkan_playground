#pragma once
#include "components/MetaBuffer.h"
#include "components/MetaPipeline.h"
#include "components/TextureManager.h"
#include "components/VulkanUtils.h"
#include "structs/UniformBuffer.h"
#include <cstdint>
#include <cstring>
#include <glm/fwd.hpp>
#include <unordered_map>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

class VQDevice;
class MeshRenderer;
class MeshRenderManager {
      public:
        enum class RenderPipeline { Generic };

        void Render() {}

        void RegisterMeshRenderer(MeshRenderer* meshRenderer, RenderPipeline pipeline) {
                _meshes[pipeline].push_back(meshRenderer);
        }

        void PrepareRendering(
                uint32_t numDescriptorSets,
                const std::unique_ptr<TextureManager>& textureManager,
                VkExtent2D swapchainExtent,
                VkRenderPass renderPass,
                std::shared_ptr<VQDevice> device
        );

        void UpdateUniformBuffers(int32_t frameIndex, glm::mat4 view, glm::mat4 proj);

        void RecordRenderCommands(VkCommandBuffer commandBuffer, int currentFrame);

      private:
        struct MeshRenderData {
                MeshRenderer* renderer;
                MetaBuffer vertexBuffer;
                MetaBuffer indexBuffer;
        };

        std::unordered_map<RenderPipeline, std::vector<MeshRenderer*>> _meshes;

        std::unordered_map<MeshRenderer*, MeshRenderData> _renderData;

        MetaPipeline _pipeline;
        std::vector<GenerticMetaPipelineUniformBuffer> _uniformBuffers; // TODO: this is pipeline-specific.

        //TODO: support dynamic addition/deletion of meshRenderer.
};