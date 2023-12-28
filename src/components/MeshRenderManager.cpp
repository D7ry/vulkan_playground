#include "MeshRenderManager.h"
#include "MeshRenderer.h"
#include "components/MetaPipeline.h"
#include "lib/VQDevice.h"
#include "lib/VQUtils.h"
#include <unordered_map>
#include <vulkan/vulkan_core.h>

static uint32_t dynamicAlignment; // TODO: fix this hack

void MeshRenderManager::PrepareRendering(
        uint32_t numFrameInFlight,
        VkRenderPass renderPass,
        std::shared_ptr<VQDevice> device
) {
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
                        RenderGroup group;
                        group.meshRenderers = it.second;
                        group.texturePath = it.second[0]->textureFilePath;
                        group.dynamicUboCount = it.second.size();
                        VQUtils::meshToBuffer(
                                it.second[0]->meshFilePath.data(), *device, group.vertexBuffer, group.indexBuffer
                        );
                        renderData.renderGroups.push_back(group);
                }
        }
        // construct meta pipelines
        // TODO: the following should be a part of the loop.
        VkPhysicalDeviceProperties properties;
        vkGetPhysicalDeviceProperties(device->physicalDevice, &properties);
        size_t minUboAlignment = properties.limits.minUniformBufferOffsetAlignment;

        dynamicAlignment = sizeof(UniformBuffer_Dynamic);
        if (minUboAlignment > 0) {
                dynamicAlignment = (dynamicAlignment + minUboAlignment - 1) & ~(minUboAlignment - 1);
        }

        size_t staticAlignment = sizeof(UniformBuffer_Static);

        // uniform buffer allocation
        {
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
                                                staticAlignment,
                                                VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                                                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
                                                        | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
                                        );
                                        // allocate dynamic ubo
                                        group.dynamicUbo[i] = device->CreateBuffer(
                                                dynamicAlignment * group.dynamicUboCount,
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
                for (auto& it : this->_runtimeRenderData) {
                        RuntimeRenderData& renderData = it.second;
                        // TODO: shader should be a part of metapipeline
                        renderData.metaPipeline = CreateGenericMetaPipeline(
                                device,
                                numFrameInFlight,
                                staticAlignment,
                                dynamicAlignment,
                                renderData.renderGroups,
                                "../shaders/vert_test.vert.spv",
                                "../shaders/frag_test.frag.spv",
                                renderPass
                        );
                        
                }
        }
}
void MeshRenderManager::UpdateUniformBuffers(int32_t frameIndex, glm::mat4 view, glm::mat4 proj) {
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

                                // vkCmdDraw(commandBuffer, 3, 1, 0, 0);
                                uint32_t indexCount = static_cast<uint32_t>(
                                        renderGroup.indexBuffer.size / sizeof(uint32_t)
                                ); // TODO: fix this hack
                                vkCmdDrawIndexed(commandBuffer, indexCount, 1, 0, 0, 0);
                        }
                }
        }
        // bind graphics pipeline
        // vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, _pipeline.pipeline);

        //        auto meshRenderers_generic = _rendererToProcess[RenderMethod::Generic];
        // for (int i = 0; i < meshRenderers_generic.size(); i++) {
        //         MeshRenderData& renderData = _renderData[meshRenderers_generic[i]];
        //         VkBuffer vertexBuffers[] = {renderData.vertexBuffer.buffer};
        //         VkDeviceSize offsets[] = {0};

        //         //TODO: group meshes of identical vertex and index buffers together so we don't need to perform
        //         repetitive bindings. vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);

        //         vkCmdBindIndexBuffer(commandBuffer, renderData.indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT32);

        //         uint32_t dynamicOffset = i * static_cast<uint32_t>(dynamicAlignment);
        //         vkCmdBindDescriptorSets(
        //                 commandBuffer,
        //                 VK_PIPELINE_BIND_POINT_GRAPHICS,
        //                 _pipeline.pipelineLayout,
        //                 0,
        //                 1,
        //                 &_pipeline.descriptorSets[currentFrame], //TODO: here we can choose which descriptor set to
        //                 bind to, so theoreically we can have different textures for the same pipeline 1,
        //                 &dynamicOffset
        //         );

        //         // issue the draw command
        //         // vkCmdDraw(commandBuffer, 3, 1, 0, 0);
        //         uint32_t indexCount = static_cast<uint32_t>(renderData.indexBuffer.size / sizeof(uint32_t)); //TODO:
        //         fix this hack vkCmdDrawIndexed(
        //                 commandBuffer, indexCount, 1, 0, 0, 0
        //         );
        // }
}

// loop for different pipelines -> loop for different textures -> loop for different dynamic buffers.