#pragma once
#include <map>
#include <vulkan/vulkan_core.h>

#include "ecs/System.h"
#include "ecs/component/ModelComponent.h"
#include "ecs/component/PhongMeshInstanceComponent.h"
#include "ecs/component/TransformComponent.h"
#include "structs/UniformBuffer.h"

#define TEXTURE_ARRAY_SIZE 8
struct PhongMesh
{
    VQBuffer vertexBuffer;
    VQBufferIndex indexBuffer;
};


struct PhongUBOStatic
{
    glm::mat4 view;
    glm::mat4 proj;
};

struct PhongUBODynamic
{
    glm::mat4 model;
    int textureOffset;
};

// self-contained system to render phong meshes
class PhongRenderSystem : public ISystem
{
  public:
    PhongMeshInstanceComponent* MakePhongMeshInstanceComponent(
        const std::string& meshPath,
        const std::string& texturePath
    );

    virtual void Init(const InitData* initData) override;
    virtual void Tick(const TickData* tickData) override;
    virtual void Cleanup() override;

  private:
    const char* VERTEX_SHADER_SRC = "../shaders/vert_test.vert.spv";
    const char* FRAGMENT_SHADER_SRC = "../shaders/frag_test.frag.spv";

    VkRenderPass _renderPass = VK_NULL_HANDLE;
    VkPipeline _pipeline = VK_NULL_HANDLE;
    VkPipelineLayout _pipelineLayout = VK_NULL_HANDLE;

    // sysetm holds layout and pool
    VkDescriptorSetLayout _descriptorSetLayout
        = VK_NULL_HANDLE; // describes layout of descriptor set
    VkDescriptorPool _descriptorPool
        = VK_NULL_HANDLE; // TODO: can we use a global descriptor pool?

    struct UBO
    {
        VQBuffer staticUBO;
        VQBuffer dynamicUBO;
    };

    std::array<VkDescriptorSet, NUM_INTERMEDIATE_FRAMES> _descriptorSets;
    std::array<UBO, NUM_INTERMEDIATE_FRAMES> _UBO;
    void resizeDynamicUbo(
        size_t dynamicUboCount,
        size_t dynamicUboSize = sizeof(PhongUBODynamic)
    );

    VQDevice* _device = nullptr;

    void createRenderPass(VQDevice* device, VkFormat swapChainImageFormat);

    // initialize resources for graphics pipeline,
    // and the pipeline itself
    void createGraphicsPipeline();

    std::unordered_map<std::string, PhongMesh> _meshes;

    TextureManager* _textureManager;

    // a huge array of texture descriptors that gets filled up as textures are loaded in
    std::array<VkDescriptorImageInfo, TEXTURE_ARRAY_SIZE> _textureDescriptorInfo;
    size_t _textureDescriptorInfoIdx = 0; // idx to the current texture descriptor that can be written in
    std::unordered_map<std::string, int> _textureDescriptorIndices; // texture name, index into the texture descriptor array
};
