#pragma once
#include <unordered_set>
#include <vulkan/vulkan_core.h>

#include "lib/VQBuffer.h"

#include "ecs/System.h"
#include "ecs/component/PhongRenderSystemInstancedComponent.h"
#include "ecs/component/TransformComponent.h"


struct PhongMeshInstanced
{
    VQBuffer vertexBuffer;
    VQBufferIndex indexBuffer;
};

// phong render system that provides CPU O(#of unique mesh) performance per draw calls
// as opposed to O(# of unique mesh instance)
// So is achieved by:
// for each mesh, store all instance-unique data into a huge buffer,
// when rendering, the GPU direcly index into the buffer.
// 
// The buffer is updated from the CPU, only if the instance data actually changed.
class PhongRenderSystemInstanced : public IRenderSystem
{
  public:
    PhongRenderSystemInstancedComponent*
    MakePhongRenderSystemInstancedComponent(
        const std::string& meshPath,
        const std::string& texturePath,
        size_t instanceNumber // initial # of instance allocated on the GPU.
                              // make this number big if intend to draw a grass field or sth.
    );

    // destroy a mesh instance component that's initialized with
    // `MakePhongRenderSystemInstancedComponent` freeing the pointer
    void DestroyPhongMeshInstanceComponent(PhongRenderSystemInstancedComponent*& component);

    virtual void Init(const InitContext* initData) override;
    virtual void Tick(const TickContext* tickData) override;

    virtual void Cleanup() override;

    // flag the entity as dirty, so that its buffer will be flushed
    void FlagAsDirty(
        Entity* entity
    );

  private:
    // spir-v source to vertex and fragment shader, relative to compiled binary
    const char* VERTEX_SHADER_SRC = "../shaders/phong_instancing.vert.spv";
    const char* FRAGMENT_SHADER_SRC = "../shaders/phong_instancing.frag.spv";

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

    struct MeshData
    {
        PhongMeshInstanced mesh;
        // buffer array to store mesh instance data
        std::array<VQBuffer, NUM_FRAME_IN_FLIGHT> instanceBuffer; // instance buffer for each frame
        int instanceBufferInstanceCount; // number of maximum supported mesh instance
        int availableInstanceBufferIdx
            = 0; // which instance buffer is currently available?
                 // increment this when assigning new instance buffer
                 // also can use as the # of instance buffer used
        std::unordered_set<PhongRenderSystemInstancedComponent*>
            components; // what components corresponds to this mesh?
                        // useful for updating the buffer field in the
                        // components
    };

    // all phong meshes created
    std::unordered_map<std::string, MeshData> _meshes;

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
        _textureDescriptorIndices; // texture name, index into the texture
                                   // descriptor array
    void updateTextureDescriptorSet(
    ); // flush the `_textureDescriptorInfo` into device, updating the
       // descriptor set

    enum class BindingLocation : unsigned int
    {
        UBO_STATIC_ENGINE = 0,
        UBO_DYNAMIC = 1,
        TEXTURE_SAMPLER = 2
    };
};
