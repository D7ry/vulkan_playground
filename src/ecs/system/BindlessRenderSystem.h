#pragma once
#include <unordered_set>
#include <vulkan/vulkan_core.h>

#include "lib/VQBuffer.h"

#include "ecs/System.h"
#include "ecs/component/TransformComponent.h"
#include "ecs/component/BindlessRenderSystemComponent.h"

// bindless render system that provides CPU O(1) performance per tick
// as opposed to O(# of unique mesh instance), or O(# of unique mesh model)
class BindlessRenderSystem : public IRenderSystem
{
    // data structure per bindless system instance
    // lives on the global instance array
    struct BindlessInstanceData
    {
        glm::mat4 model;
        float transparency;
        struct {
            unsigned int albedo; // currently we only have albedo tex
        } textureOffsets;
    };

  public:
    BindlessRenderSystemComponent* MakeComponent(
        const std::string& meshPath,
        const std::string& texturePath,
        size_t instanceNumber // initial # of instance allocated on the GPU.
                              // make this number big if intend to draw a grass field or sth.
    );

    virtual void Init(const InitContext* initData) override;
    virtual void Tick(const TickContext* tickData) override;

    virtual void Cleanup() override;

    // flag the entity as dirty, so that its buffer will be flushed
    void FlagAsDirty(Entity* entity);

  private:
    // spir-v source to vertex and fragment shader, relative to compiled binary
    const char* VERTEX_SHADER_SRC = "../shaders/bindless.vert.spv";
    const char* FRAGMENT_SHADER_SRC = "../shaders/bindless.frag.spv";

    std::array<std::vector<Entity*>, NUM_FRAME_IN_FLIGHT> _bufferUpdateQueue;

    // pipeline
    VkPipeline _pipeline = VK_NULL_HANDLE;
    VkPipelineLayout _pipelineLayout = VK_NULL_HANDLE;

    // sysetm holds its own descriptor pool
    VkDescriptorSetLayout _descriptorSetLayout = VK_NULL_HANDLE;
    VkDescriptorPool _descriptorPool = VK_NULL_HANDLE;

    std::array<VkDescriptorSet, NUM_FRAME_IN_FLIGHT> _descriptorSets;

    VQDevice* _device = nullptr;

    // initialize resources for graphics pipeline,
    // - shaders
    // - descriptors(pool, layout, sets)
    // and the pipeline itself
    void createGraphicsPipeline(const VkRenderPass renderPass, const InitContext* initData);

    // NOTE: In the future we may be able to break up the instance lookup array
    // into smaller chunks, each indexed by a draw cmd. This gives more freedom
    // to reallocate the instance lookup array.
    // this additional layer of indirection may be useful for object GC
    struct MeshData
    {
        // used to index into draw cmd
        unsigned int drawCmdOffset;
        // the draw cmd has the following fields that can be updated:
        // total instance number
        // starting instance number -- useful
        // for offsetting into the instance table when drawing
        // -- the following we usually don't modify
        // vertex buffer offset into the vertex buffer array
        // index buffer offset into the index buffer array
        // # of indices to use from the index buffer
    };

    // all phong meshes created
    std::unordered_map<std::string, MeshData> _meshes;

    TextureManager* _textureManager;

    // an array of texture descriptors that gets filled up as textures are
    // loaded in
    std::array<VkDescriptorImageInfo, TEXTURE_ARRAY_SIZE> _textureDescriptorInfo;
    size_t _textureDescriptorInfoIdx
        = 0; // idx to the current texture descriptor that can be written in,
             // also represents the total # of valid texture descriptors,
             // starting from the beginning of the _textureDescriptorInfo array
    std::unordered_map<std::string, int> _textureDescriptorIndices; // texture name, index into the
                                                                    // texture descriptor array
    void updateTextureDescriptorSet(); // flush the `_textureDescriptorInfo` into device, updating
                                       // the descriptor set

    enum class BindingLocation : unsigned int
    {
        UBO_STATIC_ENGINE = 0,
        UBO_DYNAMIC = 1,
        TEXTURE_SAMPLER = 2
    };

    // huge array containing all draw data of all instances
    std::array<VQBuffer, NUM_FRAME_IN_FLIGHT> _instanceDataArray;
    // SSBO the maps (offseted) instance IDs to actual instance datas
    std::array<VQBuffer, NUM_FRAME_IN_FLIGHT> _instanceLookupArray;
};
