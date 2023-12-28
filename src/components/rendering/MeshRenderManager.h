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


/**
 * @brief A render group is a set of meshes that shares the same texture, render pipeline, and therefore shaders. 
 * Each mesh has its own dynamic UBO for information such as transform.
 * There are exactly NUMFRAMEINFLIGHT static UBO and dynamic UBO.
  * Render groups that share the render pipeline must share the same buffer layout.
 */
struct RenderGroup {
        std::string texturePath;
        uint32_t dynamicUboCount;
        std::vector<MeshRenderer*> meshRenderers;
        VQBuffer vertexBuffer;
        VQBuffer indexBuffer;
        int indexCount;

        std::vector<VQBuffer> staticUbo;
        std::vector<VQBuffer> dynamicUbo;  
        std::vector<VkDescriptorSet> descriptorSets;

        VkDevice descriptorSetDevice;
        VkDescriptorPool descriptorPool;

        inline void cleanup() {
                vertexBuffer.Cleanup();
                indexBuffer.Cleanup();
                for (VQBuffer& buffer : staticUbo) {
                        buffer.Cleanup();
                }
                for (VQBuffer& buffer : dynamicUbo) {
                        buffer.Cleanup();
                }
                vkFreeDescriptorSets(descriptorSetDevice,descriptorPool, descriptorSets.size(), descriptorSets.data());
        }
};

class MeshRenderManager {
      public:
        enum class RenderMethod { Generic };

        MeshRenderManager() {};

        void Render() {}

        void RegisterMeshRenderer(MeshRenderer* meshRenderer, RenderMethod renderMethod) {
                _rendererToProcess[renderMethod].push_back(meshRenderer);
        }

        void PrepareRendering(
                uint32_t numFrameInFlight,
                VkRenderPass renderPass,
                std::shared_ptr<VQDevice> device
        );

        void UpdateUniformBuffers(int32_t frameIndex, glm::mat4 view, glm::mat4 proj);

        void RecordRenderCommands(VkCommandBuffer commandBuffer, int currentFrame);

        void Cleanup();

      private:
        struct RuntimeRenderData {
                MetaPipeline metaPipeline;
                std::vector<RenderGroup> renderGroups;
        };

        std::unordered_map<RenderMethod, std::vector<MeshRenderer*>> _rendererToProcess; // list of renderers to prepare for rendering

        std::unordered_map<RenderMethod, RuntimeRenderData> _runtimeRenderData; // pipeline type -> pipeline objects
        //TODO: support dynamic addition/deletion of meshRenderer.
};