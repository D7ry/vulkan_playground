#pragma once
#include <map>
#include <vulkan/vulkan_core.h>

#include "lib/VQBuffer.h"

#include "ecs/System.h"
#include "ecs/component/PhongMeshInstanceComponent.h"
#include "ecs/component/TransformComponent.h"
#include "structs/UniformBuffer.h"

// a blinn-phong mesh that lives on GPU
// TODO: should it be a part of this class?
struct PhongMesh
{
    VQBuffer vertexBuffer;
    VQBufferIndex indexBuffer;
};

// static UBO of phong render, we use the same UBO for all mesh instances
struct PhongUBOStatic
{
    glm::mat4 view;
    glm::mat4 proj;
};

// dynamic UBO of phong render, every mesh instance has its own dynamic UBO
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
    // spir-v source to vertex and fragment shader, relative to compiled binary
    const char* VERTEX_SHADER_SRC = "../shaders/phong.vert.spv";
    const char* FRAGMENT_SHADER_SRC = "../shaders/phong.frag.spv";

    VkRenderPass _renderPass = VK_NULL_HANDLE;
    VkPipeline _pipeline = VK_NULL_HANDLE;
    VkPipelineLayout _pipelineLayout = VK_NULL_HANDLE;

    // sysetm holds its own descriptor pool
    VkDescriptorSetLayout _descriptorSetLayout = VK_NULL_HANDLE;
    VkDescriptorPool _descriptorPool = VK_NULL_HANDLE;

    struct UBO
    {
        VQBuffer staticUBO;
        VQBuffer dynamicUBO;
    };

    size_t _numDynamicUBO;  // how many dynamic UBOs do we have
    size_t _currDynamicUBO; // dynamic ubo that is to be allocated
    size_t _dynamicUBOAlignmentSize; // actual size of the dynamic UBO that satisfies device alignment

    std::array<VkDescriptorSet, NUM_INTERMEDIATE_FRAMES> _descriptorSets;
    std::array<UBO, NUM_INTERMEDIATE_FRAMES> _UBO;
    void resizeDynamicUbo(size_t dynamicUboCount);

    VQDevice* _device = nullptr;

    void createRenderPass(VQDevice* device, VkFormat swapChainImageFormat);

    // initialize resources for graphics pipeline,
    // - shaders
    // - descriptors(pool, layout, sets)
    // and the pipeline itself
    void createGraphicsPipeline();

    // all phong meshes created
    std::unordered_map<std::string, PhongMesh> _meshes;

    TextureManager* _textureManager;

    // an array of texture descriptors that gets filled up as textures are
    // loaded in
    // TODO: adaptively unload textures?
    std::array<VkDescriptorImageInfo, TEXTURE_ARRAY_SIZE>
        _textureDescriptorInfo;
    size_t _textureDescriptorInfoIdx
        = 0; // idx to the current texture descriptor that can be written in,
             // also represents the total # of valid texture descriptors,
             // starting from the beginning of the _textureDescriptorInfo array
    std::unordered_map<std::string, int>
        _textureDescriptorIndices; // texture name, index into the texture
                                   // descriptor array
                                   //
    void updateTextureDescriptorSet(
    ); // flush the `_textureDescriptorInfo` into device, updating the
       // descriptor set

    enum class BindingLocation : unsigned int
    {
        UBO_STATIC = 0,
        UBO_DYNAMIC = 1,
        TEXTURE_SAMPLER = 2
    };
};
