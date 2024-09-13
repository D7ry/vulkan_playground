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
        } textureOffsets;

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

  public:
    // as a POC we don't support dynamic resizing yet
    // TODO: support dynamic resizing
    void InitMesh(const std::string& meshPath, unsigned int instanceNumber) {
        DeletionStack del;
        // load mesh into vertex and index buffer
        std::vector<Vertex> vertices;
        std::vector<uint32_t> indices;
        CoreUtils::loadModel(meshPath.c_str(), vertices, indices);

        VkDeviceSize vertexBufferSize = sizeof(Vertex) * vertices.size();
        VkDeviceSize indexBufferSize = sizeof(uint32_t) * indices.size();

        // make staging buffer for both vertex and index buffer
        std::pair<VkBuffer, VkDeviceMemory> res
            = CoreUtils::createVulkanStagingBuffer(
                _device->physicalDevice,
                _device->logicalDevice,
                vertexBufferSize + indexBufferSize
            );
        VkBuffer stagingBuffer = res.first;
        VkDeviceMemory stagingBufferMemory = res.second;
        del.push([this, stagingBuffer, stagingBufferMemory]() {
            vkDestroyBuffer(_device->logicalDevice, stagingBuffer, nullptr);
            vkFreeMemory(_device->logicalDevice, stagingBufferMemory, nullptr);
        });

        // copy vertex and index buffer to staging buffer
        void* vertexBufferDst;
        void* indexBufferDst;
        vkMapMemory(
            _device->logicalDevice,
            stagingBufferMemory,
            0,
            vertexBufferSize,
            0,
            &vertexBufferDst
        );
        vkMapMemory(
            _device->logicalDevice,
            stagingBufferMemory,
            vertexBufferSize,
            indexBufferSize,
            0,
            &indexBufferDst
        );
        memcpy(vertexBufferDst, vertices.data(), (size_t)vertexBufferSize);
        memcpy(indexBufferDst, indices.data(), (size_t)indexBufferSize);

        vkUnmapMemory(_device->logicalDevice, stagingBufferMemory);

        // copy from staging buffer to vertex/index buffer array
        CoreUtils::copyVulkanBuffer(
            _device->logicalDevice,
            _device->graphicsCommandPool,
            _device->graphicsQueue,
            stagingBuffer,
            _vertexBuffers.buffer,
            vertexBufferSize,
            0,
            _vertexBuffersWriteOffset
        );

        CoreUtils::copyVulkanBuffer(
            _device->logicalDevice,
            _device->graphicsCommandPool,
            _device->graphicsQueue,
            stagingBuffer,
            _indexBuffers.buffer,
            indexBufferSize,
            vertexBufferSize, // skip the vertex buffer region in staging buffer
            _indexBuffersWriteOffset
        );


        // create a draw command and store into `drawCommandArray`
        VkDrawIndexedIndirectCommand cmd{};
        cmd.firstIndex = _indexBuffersWriteOffset / sizeof(unsigned int);
        cmd.indexCount = indices.size();
        cmd.vertexOffset = _vertexBuffersWriteOffset / sizeof(Vertex);
        cmd.instanceCount = 0; // draw 0 instance by default
        cmd.firstInstance = _instanceLookupArrayOffset / sizeof(unsigned int);

        // reserve `instanceNumber` * sizeof(unsigned int) in
        // instanceLookupArray
        // for now, simply bump the offset
        _instanceLookupArrayOffset += instanceNumber * sizeof(unsigned int);
        
        // bump offset for next writes
        _vertexBuffersWriteOffset += vertexBufferSize;
        _indexBuffersWriteOffset += indexBufferSize;
    }

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

    void createDrawCmd(int totalInstance) {}
};
