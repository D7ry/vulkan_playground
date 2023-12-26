#include "MeshRenderer.h"
#include <vulkan/vulkan_core.h>
#include "MeshRenderManager.h"
static uint32_t dynamicAlignment; //TODO: fix this hack
void MeshRenderManager::PrepareRendering(
        VkPhysicalDevice physicalDevice,
        VkDevice logicalDevice,
        uint32_t numDescriptorSets,
        const std::unique_ptr<TextureManager>& textureManager,
        VkExtent2D swapchainExtent,
        VkRenderPass renderPass,
        VkCommandPool pool,
        VkQueue queue
) {
        // construct meta pipelines
        // TODO: the following should be a part of the loop.
        VkPhysicalDeviceProperties properties;
        vkGetPhysicalDeviceProperties(physicalDevice, &properties);
        size_t minUboAlignment = properties.limits.minUniformBufferOffsetAlignment;

        dynamicAlignment = sizeof(UniformBuffer_Dynamic);
        if (minUboAlignment > 0) {
                dynamicAlignment = (dynamicAlignment + minUboAlignment - 1) & ~(minUboAlignment - 1);
        }

        size_t staticAlignment = sizeof(UniformBuffer_Static);

        _uniformBuffers.resize(numDescriptorSets);

        // TODO: refactor

        // set up dynamic and static buffers
        size_t dynamicBufferSize = dynamicAlignment * _meshes[RenderPipeline::Generic].size();
        size_t staticBufferSize = staticAlignment;
        for (int i = 0; i < numDescriptorSets; i++) {
                _uniformBuffers[i].dynamicUniformBuffer = MetaBuffer_Create(
                        logicalDevice,
                        physicalDevice,
                        dynamicBufferSize,
                        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
                );

                _uniformBuffers[i].staticUniformBuffer = MetaBuffer_Create(
                        logicalDevice,
                        physicalDevice,
                        staticAlignment,
                        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
                );
        }

        auto& meshes = _meshes[RenderPipeline::Generic];
        size_t numMeshes = meshes.size(); // for each mesh renderer of this pipeline, we make a unique dynamic buffer.
        _pipeline = CreateGenericMetaPipeline(
                logicalDevice,
                numDescriptorSets,
                "../textures/viking_room.png",
                "../shaders/vert_test.vert.spv",
                "../shaders/frag_test.frag.spv",
                swapchainExtent,
                textureManager,
                _uniformBuffers,
                numMeshes, // each mesh has its own dynamic UBO
                staticAlignment,
                dynamicAlignment,
                renderPass
        );

        // create vertex and index buffer for meshrenderer
        for (int i = 0; i < meshes.size(); i++) {
                INFO("Allocating vertex and index buffer...");
                MeshRenderData renderData;
                renderData.renderer = meshes[i];
                VulkanUtils::createVertexBuffer(
                        physicalDevice, logicalDevice, meshes[i]->vertices, renderData.vertexBuffer, pool, queue
                );
                VulkanUtils::createIndexBuffer(
                        renderData.indexBuffer, meshes[i]->indices, logicalDevice, physicalDevice, pool, queue
                );
                ASSERT(renderData.vertexBuffer.buffer != VK_NULL_HANDLE)
                ASSERT(renderData.indexBuffer.buffer != VK_NULL_HANDLE)
                _renderData[meshes[i]] = renderData;
        }
}       
void MeshRenderManager::UpdateUniformBuffers(int32_t frameIndex, glm::mat4 view, glm::mat4 proj) {
        void* bufferAddr = _uniformBuffers[frameIndex].staticUniformBuffer.bufferAddress;
        UniformBuffer_Static ubs;
        ubs.view = view;
        ubs.proj = proj;
        ubs.proj[1][1] *= -1; // invert y axis
        memcpy(bufferAddr, &ubs, sizeof(UniformBuffer_Static));

        // each dynamic uniform buffer corresponds to a mesh renderer
        auto& meshes = _meshes[RenderPipeline::Generic];
        for (int i = 0; i < meshes.size(); i++) {
                bufferAddr = reinterpret_cast<void*>(
                        reinterpret_cast<uintptr_t>(_uniformBuffers[frameIndex].dynamicUniformBuffer.bufferAddress)
                        + (i * sizeof(UniformBuffer_Dynamic))
                );
                UniformBuffer_Dynamic ubd;
                ubd.model = meshes[i]->GetModelMatrix();
                memcpy(bufferAddr, &ubd, sizeof(UniformBuffer_Dynamic));
        }
}
void MeshRenderManager::RecordRenderCommands(VkCommandBuffer commandBuffer, int currentFrame) {
        // bind graphics pipeline
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, _pipeline.pipeline);

        auto meshRenderers_generic = _meshes[RenderPipeline::Generic];
        for (int i = 0; i < meshRenderers_generic.size(); i++) {
                MeshRenderData& renderData = _renderData[meshRenderers_generic[i]];
                VkBuffer vertexBuffers[] = {renderData.vertexBuffer.buffer};
                VkDeviceSize offsets[] = {0};
                vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);

                vkCmdBindIndexBuffer(commandBuffer, renderData.indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT32);

                uint32_t dynamicOffset = i * static_cast<uint32_t>(dynamicAlignment);
                vkCmdBindDescriptorSets(
                        commandBuffer,
                        VK_PIPELINE_BIND_POINT_GRAPHICS,
                        _pipeline.pipelineLayout,
                        0,
                        1,
                        &_pipeline.descriptorSets[currentFrame],
                        1,
                        &dynamicOffset
                );

                // issue the draw command
                // vkCmdDraw(commandBuffer, 3, 1, 0, 0);
                uint32_t indexCount = static_cast<uint32_t>(renderData.indexBuffer.size / sizeof(uint32_t)); //TODO: fix this hack
                vkCmdDrawIndexed(
                        commandBuffer, indexCount, 1, 0, 0, 0
                );
        }
}
