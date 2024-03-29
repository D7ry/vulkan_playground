#pragma once
#include "MetaPipeline.h"
#include "components/TextureManager.h"
#include "lib/VQBuffer.h"
#include "structs/UniformBuffer.h"
#include <cstdint>
#include <cstring>
#include <glm/fwd.hpp>
#include <imgui.h>
#include <unordered_map>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>
class VQDevice;
class MeshInstance;

class MeshRenderManager
{
  public:
    enum class RenderMethod
    {
        Generic
    };

    MeshRenderManager(){};

    void Render() {}

    void AddMeshRenderer(MeshInstance* renderer);

    /**
     * @brief Create a Mesh instance. The creator can control the instance's
     * transform.
     *
     * The render manager handles instancing of multiple meshes.
     * @param meshFilePath path to the mesh file
     * @param textureFilePath path to the texture file
     * @return MeshInstance* pointer to the created mesh instance
     */
    MeshInstance* CreateMeshInstance(
        const char* meshFilePath,
        const char* textureFilePath
    );

    void PrepareRendering(
        uint32_t numFrameInFlight,
        VkRenderPass renderPass,
        std::shared_ptr<VQDevice> device
    );

    void UpdateUniformBuffers(
        int32_t frameIndex,
        glm::mat4 view,
        glm::mat4 proj
    );

    void RecordRenderCommands(VkCommandBuffer commandBuffer, int currentFrame);

    void Cleanup();

    void DrawImgui();

  private:
    struct RuntimeRenderData
    {
        MetaPipeline metaPipeline;
        std::vector<RenderGroup> renderGroups;
    };

    std::unordered_map<RenderMethod, std::vector<MeshInstance*>>
        _rendererToProcess; // list of renderers to prepare for rendering

    std::unordered_map<RenderMethod, RuntimeRenderData>
        _runtimeRenderData; // pipeline type -> pipeline objects
    // TODO: support dynamic addition/deletion of meshRenderer.

    std::shared_ptr<VQDevice> _device;
};
