#include "MeshRenderManager.h"
#include "MeshInstance.h"
#include "MetaPipeline.h"
#include "components/rendering/RenderGroup.h"
#include "lib/VQDevice.h"
#include "lib/VQUtils.h"
#include "structs/UniformBuffer.h"
#include <unordered_map>
#include <vulkan/vulkan_core.h>

void MeshRenderManager::PrepareRendering(
    uint32_t numFrameInFlight,
    VkRenderPass renderPass,
    std::shared_ptr<VQDevice> device
) {
    this->_device = device;
    // pipeline creation
    {
        INFO("Creating metapipeline...");
        std::vector<RenderGroup> gg;
        this->_runtimeRenderData[RenderMethod::Generic].metaPipeline
            = CreateGenericMetaPipeline(
                device,
                gg,
                "../shaders/vert_test.vert.spv",
                "../shaders/frag_test.frag.spv",
                renderPass
            );
    }
}

// TODO: optimize
void MeshRenderManager::UpdateUniformBuffers(
    int32_t frameIndex,
    glm::mat4 view,
    glm::mat4 proj
) {
    for (auto& elem : _runtimeRenderData) {
        std::vector<RenderGroup>& groups = elem.second.renderGroups;
        for (RenderGroup& group : groups) {
            //  TODO: view UBO should be the shared buffer
            UniformBuffer_Static ubs;
            ubs.view = view;
            ubs.proj = proj;
            ubs.proj[1][1] *= -1; // invert y axis
            memcpy(
                group.uniformBuffers[frameIndex].staticUBO.bufferAddress,
                &ubs,
                sizeof(UniformBuffer_Static)
            );

            // dynamic
            for (int i = 0; i < group.meshRenderers.size(); i++) {
                int bufferOffset
                    = i * static_cast<uint32_t>(group.dynamicUboSize);
                void* bufferAddr = reinterpret_cast<void*>(
                    reinterpret_cast<uintptr_t>(group.uniformBuffers[frameIndex]
                                                    .dynamicUBO.bufferAddress)
                    + bufferOffset
                );
                UniformBuffer_Dynamic ubd;
                ubd.model = group.meshRenderers[i]->GetModelMatrix();
                memcpy(bufferAddr, &ubd, sizeof(UniformBuffer_Dynamic));
            }
        }
    }
}

// TODO: optimize
void MeshRenderManager::RecordRenderCommands(
    VkCommandBuffer commandBuffer,
    int currentFrame
) {
    for (auto& elem : _runtimeRenderData) {
        RuntimeRenderData& renderData = elem.second;
        for (RenderGroup& renderGroup : renderData.renderGroups) {
            VkBuffer vertexBuffers[] = {renderGroup.vertexBuffer.buffer};
            VkDeviceSize offsets[] = {0};

            vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
            vkCmdBindIndexBuffer(
                commandBuffer,
                renderGroup.indexBuffer.buffer,
                0,
                VK_INDEX_TYPE_UINT32
            );
            MetaPipeline& pipeline = elem.second.metaPipeline;
            DEBUG_ASSERT(pipeline.pipeline != VK_NULL_HANDLE);
            vkCmdBindPipeline(
                commandBuffer,
                VK_PIPELINE_BIND_POINT_GRAPHICS,
                pipeline.pipeline
            );

            for (int i = 0; i < renderGroup.meshRenderers.size(); i++) {
                uint32_t dynamicOffset
                    = i * static_cast<uint32_t>(renderGroup.dynamicUboSize);

                MeshInstance* renderer = renderGroup.meshRenderers[i];

                vkCmdBindDescriptorSets(
                    commandBuffer,
                    VK_PIPELINE_BIND_POINT_GRAPHICS,
                    renderData.metaPipeline.pipelineLayout,
                    0,
                    1,
                    &renderGroup.descriptorSets[currentFrame],
                    1,
                    &dynamicOffset
                );

                vkCmdDrawIndexed(
                    commandBuffer,
                    renderGroup.indexBuffer.numIndices,
                    1,
                    0,
                    0,
                    0
                );
            }
        }
    }
    // }
}

void MeshRenderManager::Cleanup() {
    INFO("CLeaning up mesh render manager...");
    for (auto& elem : _runtimeRenderData) {
        RuntimeRenderData& data = elem.second;
        for (auto& renderGroup : data.renderGroups) {
            renderGroup.cleanup();
        }
        data.metaPipeline.Cleanup();
    }
}

MeshInstance* MeshRenderManager::CreateMeshInstance(
    const char* meshFilePath,
    const char* textureFilePath
) {

    MeshInstance* mesh = new MeshInstance();
    for (auto& group : _runtimeRenderData[RenderMethod::Generic].renderGroups) {
        if (group.texturePath == textureFilePath
            && group.meshPath == meshFilePath) { // TODO: one rendergroup can
                                                 // handle multiple textures!
            group.addInstance(mesh);
            return mesh;
        }
    }
    _runtimeRenderData[RenderMethod::Generic].renderGroups.emplace(
        _runtimeRenderData[RenderMethod::Generic].renderGroups.begin(),
        CreateRenderGroup<UniformBuffer_Dynamic, UniformBuffer_Static>(
            this->_device, textureFilePath, meshFilePath, 1
        )
    );
    RenderGroup& group
        = _runtimeRenderData[RenderMethod::Generic].renderGroups[0];
    _runtimeRenderData[RenderMethod::Generic]
        .metaPipeline.AllocateDescriptorSets(group);
    group.InitUniformbuffers();
    group.addInstance(mesh);
    return mesh;
};

void MeshRenderManager::DrawImgui() {
    ImGui::Begin("MeshRenderManager");
    ImGui::Text("MeshRenderManager");
    ImGui::Text("Renderers:");
    for (auto& elem : _runtimeRenderData) {
        std::vector<RenderGroup>& groups = elem.second.renderGroups;
        for (RenderGroup& group : groups) {
            ImGui::Text(
                "Mesh: %s, Texture: %s",
                group.meshPath.data(),
                group.texturePath.data()
            );
            int i = 0;
            for (MeshInstance* renderer : group.meshRenderers) {
                ImGui::Text("Renderer %d", i);
                renderer->DrawImguiController();
                i++;
            }
            i = 0;
            ImGui::Separator();
        }
    }
    ImGui::End();
}
