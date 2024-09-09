#pragma once
#include "ecs/System.h"
#include "lib/VQBuffer.h"

// FIXME: this can be generalized into a "LineSystem" that draws line instances
// a global coorindate grid similar to blender's background
class GlobalGridSystem : public ISingletonSystem, IRenderSystem
{
  public:
    struct UBOStatic
    {
        glm::vec3 gridColor; // rgb
    };
    struct UBOStaticDevice
    {
        VQBuffer buffer;
    };

    virtual void Init(const InitContext* initData) override;

    virtual void Tick(const TickContext* tickData) override;

    virtual void Cleanup() override;

  private:
    const char* VERTEX_SHADER_SRC = "../shaders/global_grid.vert.spv";
    const char* FRAGMENT_SHADER_SRC = "../shaders/global_grid.frag.spv";
    void createGraphicsPipeline(const VkRenderPass renderPass, const InitContext* initData);
    enum class BindingLocation : unsigned int
    {
        UBO_STATIC_ENGINE= 0,
        UBO_STATIC = 1
    };

    VkPipeline _pipeline = VK_NULL_HANDLE;
    VkPipelineLayout _pipelineLayout = VK_NULL_HANDLE;

    // sysetm holds its own descriptor pool
    VkDescriptorSetLayout _descriptorSetLayout = VK_NULL_HANDLE;
    VkDescriptorPool _descriptorPool = VK_NULL_HANDLE;

    std::array<VkDescriptorSet, NUM_FRAME_IN_FLIGHT> _descriptorSets;
    std::array<UBOStaticDevice, NUM_FRAME_IN_FLIGHT> _UBO;

    VQDevice* _device;

    struct {
        VQBuffer vertexBuffer;
        VQBufferIndex indexBuffer;
    } _gridMesh;

    size_t _numLines;
};
