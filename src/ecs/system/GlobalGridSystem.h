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
        glm::mat4 view;
        glm::mat4 proj;
        glm::vec3 gridColor; // rgb
    };
    struct UBOStaticDevice
    {
        VQBuffer buffer;
    };

    virtual void Init(const InitData* initData) override;

    virtual void Tick(const TickData* tickData) override;

    virtual void Cleanup() override;

  private:
    const char* VERTEX_SHADER_SRC = "../shaders/global_grid.vert.spv";
    const char* FRAGMENT_SHADER_SRC = "../shaders/global_grid.frag.spv";
    void createGraphicsPipeline(const VkRenderPass renderPass);
    enum class BindingLocation : unsigned int
    {
        UBO_STATIC = 0
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
