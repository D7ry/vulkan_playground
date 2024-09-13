#pragma once
#include <unordered_set>
#include <vulkan/vulkan_core.h>

#include "components/DeletionStack.h"
#include "lib/VQBuffer.h"
#include "lib/VQUtils.h"

#include "ecs/System.h"
#include "ecs/component/BindlessRenderSystemComponent.h"
#include "ecs/component/TransformComponent.h"

// bindless render system that provides CPU O(1) performance per tick
// as opposed to O(# of unique mesh instance), or O(# of unique mesh model)
class BindlessRenderSystem : public IRenderSystem
{
    // data structure per bindless system instance
    // lives on the `instanceDataArray` buffer
    struct BindlessInstanceData
    {
        glm::mat4 model;
        float transparency;

        struct
        {
            unsigned int albedo; // currently we only have albedo tex
        } textureIndex;

        // none-render metadata
        unsigned int drawCmdIndex; // index into draw cmd
    };

    // note that we don't create NUM_FRAME_IN_FLIGHT vertex/index
    // buffers assuming synchronization is trivial
    // TODO: add synchronization protection to them.

    // all index buffers
    VQBuffer _indexBuffers;
    unsigned int _indexBuffersWriteOffset = 0;
    // all vertex buffers
    VQBuffer _vertexBuffers;
    unsigned int _vertexBuffersWriteOffset = 0;

    // buffer arrays
    struct BindlessBuffer
    {
        // huge array containing `BindlessInstanceData`
        VQBuffer instanceDataArray; // <BindlessInstanceData>
        // SSBO the maps (offseted) instance IDs to actual instance datas
        VQBuffer instanceLookupArray; // <unsigned int>
        // list of draw commands
        VQBuffer
            drawCommandArray; // <VkDrawIndexedIndirectCommand>
                              // indexCount -- how many index to draw
                              // instanceCount
                              // firstIndex -- which index to start from in
                              // `indexBuffers` firstInstance -- which index to
                              // start from in `instanceLookupArray`
    };

    std::array<BindlessBuffer, NUM_FRAME_IN_FLIGHT> _bindlessBuffers;
    unsigned int _instanceLookupArrayOffset = 0;
    unsigned int _drawCommandArrayOffset
        = 0; // offset in drawCommandArray, to which we can append a new
             // VkDrawIndexedIndirectCommand
    unsigned int _instanceDataArrayOffset = 0;

    // maps model to a vec of batches that renders the model

    struct RenderBatch
    {
        unsigned int maxSize;
        unsigned int instanceCount;
        unsigned int drawCmdOffset;
        // each render batch has its own draw command, that stores
        // additional render batch infos
    };

    std::unordered_map<std::string, std::vector<RenderBatch>> _modelBatches;

  public:
    RenderBatch createRenderBatch(
        const std::string& meshPath,
        unsigned int batchSize
    );
    BindlessRenderSystemComponent* MakeComponent(
        const std::string& meshPath,
        const std::string& texturePath
    );

    void DestroyComponent(BindlessRenderSystemComponent*& component);

    virtual void Init(const InitContext* initData) override;
    virtual void Tick(const TickContext* tickData) override;

    virtual void Cleanup() override;

    // flag the entity as dirty, so that its buffer will be flushed
    void FlagUpdate(Entity* entity);

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
    void createGraphicsPipeline(
        const VkRenderPass renderPass,
        const InitContext* initData
    );

    enum class BindingLocation : unsigned int
    {
        UBO_STATIC_ENGINE = 0,
        UBO_DYNAMIC = 1,
        TEXTURE_SAMPLER = 2
    };

    TextureManager* _textureManager;

    // an array of texture descriptors that gets filled up as textures are
    // loaded in
    std::array<VkDescriptorImageInfo, TEXTURE_ARRAY_SIZE>
        _textureDescriptorInfo;
    size_t _textureDescriptorInfoIdx
        = 0; // idx to the current texture descriptor that can be written in,
             // also represents the total # of valid texture descriptors,
             // starting from the beginning of the _textureDescriptorInfo array
    std::unordered_map<std::string, int>
        _textureDescriptorIndices; // texture name, index into the
                                   // texture descriptor array
    void updateTextureDescriptorSet(
    ); // flush the `_textureDescriptorInfo` into device, updating
       // the descriptor set

    // create resrouces required for bindless rendering. Including:
    // - huge SSBO to store all instance data
    // - a less huge SSBO to store pointers to all instance data
    // - a UBO to store parameters of draw commands
    void createBindlessResources();


    DeletionStack _deletionStack;
};
