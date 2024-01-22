#include "MeshRenderManager.h"
#include "MeshRenderer.h"
#include "MetaPipeline.h"
#include "components/rendering/RenderGroup.h"
#include "lib/VQDevice.h"
#include "lib/VQUtils.h"
#include "structs/UniformBuffer.h"
#include <unordered_map>
#include <vulkan/vulkan_core.h>

static uint32_t dynamicAlignment; // TODO: fix this hack

void MeshRenderManager::PrepareRendering(
        uint32_t numFrameInFlight,
        VkRenderPass renderPass,
        std::shared_ptr<VQDevice> device
)
{
        // construct render groups
        for (auto it = _rendererToProcess.begin(); it != _rendererToProcess.end(); it++) {
                ASSERT(it->first == RenderMethod::Generic); // TODO: add more render methods
                _runtimeRenderData[it->first] = RuntimeRenderData();
                RuntimeRenderData& renderData = _runtimeRenderData.at(it->first);
                // hash mesh + texture
                std::unordered_map<std::string, std::vector<MeshRenderer*>> uniqueModlesTextures;
                for (MeshRenderer* renderer : it->second) {
                        ASSERT(!renderer->meshFilePath.empty() && !renderer->textureFilePath.empty());
                        std::string meshTexture = renderer->meshFilePath + renderer->textureFilePath;
                        if (uniqueModlesTextures.find(meshTexture) != uniqueModlesTextures.end()) {
                                uniqueModlesTextures[meshTexture].push_back(renderer);
                        } else {
                                uniqueModlesTextures[meshTexture] = std::vector<MeshRenderer*>{renderer};
                        }
                }
                for (auto& it : uniqueModlesTextures) {
                        RenderGroup group = CreateRenderGroup<UniformBuffer_Dynamic, UniformBuffer_Static>(device);
                        group.meshRenderers = it.second;
                        group.texturePath = it.second[0]->textureFilePath;
                        group.dynamicUboCount = it.second.size();
                        VQUtils::meshToBuffer(
                                it.second[0]->meshFilePath.data(), *device, group.vertexBuffer, group.indexBuffer
                        );
                        renderData.renderGroups.push_back(group);
                }
        }

        // uniform buffer allocation
        {
                INFO("Allocating uniform buffers...");
                for (auto& it : this->_runtimeRenderData) {
                        RuntimeRenderData& renderData = it.second;
                        for (RenderGroup& group : renderData.renderGroups) {
                                ASSERT(group.staticUbo.empty());
                                ASSERT(group.dynamicUbo.empty());
                                group.staticUbo.resize(numFrameInFlight);
                                group.dynamicUbo.resize(numFrameInFlight);
                                for (int i = 0; i < numFrameInFlight; i++) {
                                        // allocate static ubo
                                        group.staticUbo[i] = device->CreateBuffer(
                                                group.staticUboSize,
                                                VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                                                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
                                                        | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
                                        );
                                        // allocate dynamic ubo
                                        group.dynamicUbo[i] = device->CreateBuffer(
                                                group.dynamicUboSize * group.dynamicUboCount,
                                                VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                                                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
                                                        | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
                                        );
                                }
                        }
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
                                numFrameInFlight,
                                renderData.renderGroups,
                                "../shaders/vert_test.vert.spv",
                                "../shaders/frag_test.frag.spv",
                                renderPass
                        );
                }
        }
}

void MeshRenderManager::UpdateUniformBuffers(int32_t frameIndex, glm::mat4 view, glm::mat4 proj)
{
        for (auto& elem : _runtimeRenderData) {
                std::vector<RenderGroup>& groups = elem.second.renderGroups;
                for (RenderGroup& group : groups) {
                        // static TODO: view UBO should be the shared buffer
                        UniformBuffer_Static ubs;
                        ubs.view = view;
                        ubs.proj = proj;
                        ubs.proj[1][1] *= -1; // invert y axis
                        memcpy(group.staticUbo[frameIndex].bufferAddress, &ubs, sizeof(UniformBuffer_Static));

                        // dynamic
                        for (int i = 0; i < group.meshRenderers.size(); i++) {
                                void* bufferAddr = reinterpret_cast<void*>(
                                        reinterpret_cast<uintptr_t>(group.dynamicUbo[frameIndex].bufferAddress)
                                        + (i * sizeof(UniformBuffer_Dynamic)
                                        ) // buffer address points to the beginning of the dynamic buffer, add offset.
                                );
                                UniformBuffer_Dynamic ubd;
                                ubd.model = group.meshRenderers[i]->GetModelMatrix();
                                memcpy(bufferAddr, &ubd, sizeof(UniformBuffer_Dynamic));
                        }
                }
        }

        // void* bufferAddr = _uniformBuffers[frameIndex].staticUniformBuffer.bufferAddress;
        // UniformBuffer_Static ubs;
        // ubs.view = view;
        // ubs.proj = proj;
        // ubs.proj[1][1] *= -1; // invert y axis
        // memcpy(bufferAddr, &ubs, sizeof(UniformBuffer_Static));

        // // each dynamic uniform buffer corresponds to a mesh renderer
        // auto& meshes = _rendererToProcess[RenderMethod::Generic];
        // for (int i = 0; i < meshes.size(); i++) {
        //         bufferAddr = reinterpret_cast<void*>(
        //                 reinterpret_cast<uintptr_t>(_uniformBuffers[frameIndex].dynamicUniformBuffer.bufferAddress)
        //                 + (i * sizeof(UniformBuffer_Dynamic)) // buffer address points to the beginning of the
        //                 dynamic buffer, add offset.
        //         );
        //         UniformBuffer_Dynamic ubd;
        //         ubd.model = meshes[i]->GetModelMatrix();
        //         memcpy(bufferAddr, &ubd, sizeof(UniformBuffer_Dynamic));
        // }
}

void MeshRenderManager::RecordRenderCommands(VkCommandBuffer commandBuffer, int currentFrame)
{
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
                                uint32_t dynamicOffset = i * static_cast<uint32_t>(dynamicAlignment);

                                MeshRenderer* renderer = renderGroup.meshRenderers[i];

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

void MeshRenderManager::Cleanup()
{
        INFO("CLeaning up mesh render manager...");
        for (auto& elem : _runtimeRenderData) {
                RuntimeRenderData& data = elem.second;
                for (auto& renderGroup : data.renderGroups) {
                        renderGroup.cleanup();
                }
                data.metaPipeline.Cleanup();
        }
}
