#pragma once
#include "MetaPipeline.h"
#include "components/TextureManager.h"
#include "lib/VQBuffer.h"
#include "structs/UniformBuffer.h"
#include <cstdint>
#include <cstring>
#include <glm/fwd.hpp>
#include <unordered_map>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

class VQDevice;
class MeshRenderer;

class MeshRenderManager
{
      public:
        enum class RenderMethod
        {
                Generic
        };

        MeshRenderManager(){};

        void Render() {}

        void TestAddRenderer(MeshRenderer* renderer);

        void RegisterMeshRenderer(MeshRenderer* meshRenderer, RenderMethod renderMethod)
        {
                _rendererToProcess[renderMethod].push_back(meshRenderer);
        }

        void PrepareRendering(uint32_t numFrameInFlight, VkRenderPass renderPass, std::shared_ptr<VQDevice> device);

        void UpdateUniformBuffers(int32_t frameIndex, glm::mat4 view, glm::mat4 proj);

        void RecordRenderCommands(VkCommandBuffer commandBuffer, int currentFrame);

        void Cleanup();

      private:
        struct RuntimeRenderData
        {
                MetaPipeline metaPipeline;
                std::vector<RenderGroup> renderGroups;
        };

        std::unordered_map<RenderMethod, std::vector<MeshRenderer*>>
                _rendererToProcess; // list of renderers to prepare for rendering

        std::unordered_map<RenderMethod, RuntimeRenderData> _runtimeRenderData; // pipeline type -> pipeline objects
        // TODO: support dynamic addition/deletion of meshRenderer.
};