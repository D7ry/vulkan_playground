#pragma once
#include <unordered_set>
#include <vulkan/vulkan_core.h>

#include "lib/VQBuffer.h"

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
        } textureOffsets;

        // none-render metadata
        unsigned int drawCmdIndex; // index into draw cmd
    };

    // buffer arrays
    struct BindlessBuffer
    {
        // huge array containing `BindlessInstanceData`
        VQBuffer instanceDataArray; // <BindlessInstanceData>
        // SSBO the maps (offseted) instance IDs to actual instance datas
        VQBuffer instanceLookupArray; // <unsigned int>
        // list of draw commands
        VQBuffer drawCommandArray; // <VkDrawIndirectCommand>
                                   // vertexCount
                                   // instanceCount
                                   // firstVertex
                                   // firstInstance
    };

    std::array<BindlessBuffer, NUM_FRAME_IN_FLIGHT> _bindlessBuffers;

  public:
    BindlessRenderSystemComponent* MakeComponent(
        const std::string& meshPath,
        const std::string& texturePath,
        size_t instanceNumber // initial # of instance allocated on the GPU.
                              // make this number big if intend to draw a grass
                              // field or sth. NOTE: currently dynamic
                              // upsizing&shrinking hasn't been implemented.
                              // make this number the number the # of instances
                              // you wish to draw
                              //
                              // TODO: implement dynamic resizing
    );

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

    // NOTE: In the future we may be able to break up the instance lookup array
    // into smaller chunks, each indexed by a draw cmd. This gives more freedom
    // to reallocate the instance lookup array.
    // this additional layer of indirection may be useful for object GC
    struct MeshData
    {
        // used to index into draw cmd/modify draw cmd
        unsigned int drawCmdOffset;
        unsigned int
            drawCmdFirstInstance; // for quick lookup of
                                  // the first instance.
                                  // note this must be manually synced
                                  // with the first instance in the draw command
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
    std::array<VkDescriptorImageInfo, TEXTURE_ARRAY_SIZE>
        _textureDescriptorInfo;
    std::unordered_map<std::string, int>
        _textureDescriptorIndices; // texture name, index into the
                                   // texture descriptor array
    void updateTextureDescriptorSet(
    ); // flush the `_textureDescriptorInfo` into device, updating
       // the descriptor set
    /**
     *
     * Let "DrawGroup" be a draw cmd, associated with 
     * 1. a set of vertex and index buffer offset(i.e.a mesh),
     * 2. a reserved region in the `instanceDataArray`
     * 3. a reserved region in the `instanceLookupArray`
     *
     * Think of a draw group as a vm page.
     */

    struct DrawGroup
    {

    };

    // create resrouces required for bindless rendering. Including:
    // - huge SSBO to store all instance data
    // - a less huge SSBO to store pointers to all instance data
    // - a UBO to store parameters of draw commands
    void createBindlessResources();


    // utility methods
    void updateDrawCmd(
        const MeshData& meshData,
        vk::DrawIndirectCommand command
    ) {
        // meshData.drawCmdOffset

    };



    void createDrawCmd(int totalInstance ) {}
};
