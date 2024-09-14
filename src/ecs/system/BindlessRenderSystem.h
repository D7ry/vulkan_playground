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
    /* ---------- SSBO Data Types ---------- */

    // https://docs.vulkan.org/guide/latest/shader_memory_layout.html
    static const unsigned int SSBO_INSTANCE_DATA_ALIGNMENT = 16; // OpDecorate %array140 ArrayStride 16
    static const unsigned int SSBO_INSTANCE_INDEX_ALIGNMENT = 4; // OpDecorate %array430 ArrayStride 4
    using SSBOInstanceIndex = unsigned int;
    static_assert(sizeof(SSBOInstanceIndex) % SSBO_INSTANCE_INDEX_ALIGNMENT == 0);
    // data structure per bindless system instance
    // lives on the `instanceDataArray` buffer
    struct SSBOInstanceData
    {
        glm::mat4 model;
        float transparency;

        struct
        {
            int albedo; // currently we only have albedo tex
        } textureIndex;

        // none-render metadata
        const unsigned int drawCmdIndex; // index into draw cmd

        const uint32_t padding[1];
    };
    static_assert(sizeof(SSBOInstanceData) % SSBO_INSTANCE_DATA_ALIGNMENT == 0);


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
        VQBuffer instanceIndexArray; // <InstanceIndexArrayIndexType>
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
    unsigned int _instanceIndexArrayOffset = 0;
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
    void updateMeshInstanceData(int frame);
    // spir-v source to vertex and fragment shader, relative to compiled binary
    const char* VERTEX_SHADER_SRC = "../shaders/bindless.vert.spv";
    const char* FRAGMENT_SHADER_SRC = "../shaders/bindless.frag.spv";

    // list of functions that updates each frame in flight
    std::array<std::vector<std::function<void()>>, NUM_FRAME_IN_FLIGHT> _updateQueue;

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
        INSTANCE_DATA = 1,
        INSTANCE_INDEX = 2,
        TEXTURE_SAMPLER = 3
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
    
    // flush the `_textureDescriptorInfo` into device, updating the descriptor set
    void updateTextureDescriptorSet(int frame);

    // create resrouces required for bindless rendering. Including:
    // - huge SSBO to store all instance data
    // - a less huge SSBO to store pointers to all instance data
    // - a UBO to store parameters of draw commands
    void createBindlessResources();

    DeletionStack _deletionStack;
};
