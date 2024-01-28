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
    // construct render groups
    for (auto it = _rendererToProcess.begin(); it != _rendererToProcess.end(); it++) {
        ASSERT(it->first == RenderMethod::Generic); // TODO: add more render methods
        _runtimeRenderData[it->first] = RuntimeRenderData();
        RuntimeRenderData& renderData = _runtimeRenderData.at(it->first);
        // hash mesh + texture
        std::unordered_map<std::string, std::vector<MeshInstance*>> uniqueModlesTextures;
        for (MeshInstance* renderer : it->second) {
            ASSERT(!renderer->meshFilePath.empty() && !renderer->textureFilePath.empty());
            std::string meshTexture = renderer->meshFilePath + renderer->textureFilePath;
            if (uniqueModlesTextures.find(meshTexture) != uniqueModlesTextures.end()) {
                uniqueModlesTextures[meshTexture].push_back(renderer);
            } else {
                uniqueModlesTextures[meshTexture] = std::vector<MeshInstance*>{renderer};
            }
        }
        for (auto& it : uniqueModlesTextures) {
            RenderGroup group = CreateRenderGroup<UniformBuffer_Dynamic, UniformBuffer_Static>(
                device, it.second[0]->textureFilePath, it.second[0]->meshFilePath, it.second.size()
            );
            group.meshRenderers = it.second;
            renderData.renderGroups.push_back(group);
        }
    }
    // pipeline creation
    {
        INFO("Creating metapipeline...");
        for (auto& it : this->_runtimeRenderData) {
            RuntimeRenderData& renderData = it.second;
            // TODO: shader should be a part of metapipeline
            renderData.metaPipeline = CreateGenericMetaPipeline(
                device,
                renderData.renderGroups,
                "../shaders/vert_test.vert.spv",
                "../shaders/frag_test.frag.spv",
                renderPass
            );

            for (auto& group : renderData.renderGroups) {
                renderData.metaPipeline.AllocateDescriptorSets(group);
                group.InitUniformbuffers();
            }
        }
    }
}

void MeshRenderManager::UpdateUniformBuffers(int32_t frameIndex, glm::mat4 view, glm::mat4 proj) {
    for (auto& elem : _runtimeRenderData) {
        std::vector<RenderGroup>& groups = elem.second.renderGroups;
        for (RenderGroup& group : groups) {
            //  TODO: view UBO should be the shared buffer
            UniformBuffer_Static ubs;
            ubs.view = view;
            ubs.proj = proj;
            ubs.proj[1][1] *= -1; // invert y axis
            memcpy(group.frameData[frameIndex].staticUBO.bufferAddress, &ubs, sizeof(UniformBuffer_Static));

            // dynamic
            for (int i = 0; i < group.meshRenderers.size(); i++) {
                int bufferOffset = i * static_cast<uint32_t>(group.dynamicUboSize);
                void* bufferAddr = reinterpret_cast<void*>(
                    reinterpret_cast<uintptr_t>(group.frameData[frameIndex].dynamicUBO.bufferAddress) + bufferOffset
                );
                UniformBuffer_Dynamic ubd;
                ubd.model = group.meshRenderers[i]->GetModelMatrix();
                memcpy(bufferAddr, &ubd, sizeof(UniformBuffer_Dynamic));
            }
        }
    }
}

void MeshRenderManager::RecordRenderCommands(VkCommandBuffer commandBuffer, int currentFrame) {
    for (auto& elem : _runtimeRenderData) {
        RuntimeRenderData& renderData = elem.second;
        for (RenderGroup& renderGroup : renderData.renderGroups) {
            VkBuffer vertexBuffers[] = {renderGroup.vertexBuffer.buffer};
            VkDeviceSize offsets[] = {0};

            vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
            vkCmdBindIndexBuffer(commandBuffer, renderGroup.indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT32);
            MetaPipeline& pipeline = elem.second.metaPipeline;
            DEBUG_ASSERT(pipeline.pipeline != VK_NULL_HANDLE);
            vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.pipeline);

            for (int i = 0; i < renderGroup.meshRenderers.size(); i++) {
                uint32_t dynamicOffset = i * static_cast<uint32_t>(renderGroup.dynamicUboSize);

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

                vkCmdDrawIndexed(commandBuffer, renderGroup.indexBuffer.numIndices, 1, 0, 0, 0);
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

void MeshRenderManager::AddMeshRenderer(MeshInstance* renderer) {
    std::string& texture = renderer->textureFilePath;
    // check if a render group exists
    for (auto& group : _runtimeRenderData[RenderMethod::Generic].renderGroups) {
        if (group.texturePath == texture) { // TODO: use a better way to hash it
            group.addRenderer(renderer);
            return;
        }
    }

    // TODO: refactor rendergroup into MeshRenderer, and MeshRenderer into meshrendererInstance.
    // For users, they have access to instance creation. resource management should be handled by the engine.
    _runtimeRenderData[RenderMethod::Generic].renderGroups.emplace(
        _runtimeRenderData[RenderMethod::Generic].renderGroups.begin(),
        CreateRenderGroup<UniformBuffer_Dynamic, UniformBuffer_Static>(
            this->_device, renderer->textureFilePath, renderer->meshFilePath, 1
        )
    );
    RenderGroup& group = _runtimeRenderData[RenderMethod::Generic].renderGroups[0];
    group.meshRenderers.push_back(renderer);
    _runtimeRenderData[RenderMethod::Generic].metaPipeline.AllocateDescriptorSets(group);
    group.InitUniformbuffers();
}
