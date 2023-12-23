#pragma once
#include "components/VulkanUtils.h"
#include <cstring>
#include <stb_image.h>
#include <vulkan/vulkan_core.h>

// TODO: use a single command buffe for higher throughput; may implement our own command buffer "buffer".
class TextureManager {

      public:
        TextureManager() = delete;
        TextureManager(
                VkPhysicalDevice physicalDevice,
                VkDevice logicalDevice,
                VkCommandPool commandPool,
                VkQueue graphicsQueue
        ) {
                _physicalDevice = physicalDevice;
                _logicalDevice = logicalDevice;
                _commandPool = commandPool;
                _graphicsQueue = graphicsQueue;
        }
        ~TextureManager() {}

        void LoadTexture(std::string& texturePath) {
                int width, height, channels;
                stbi_uc* pixels = stbi_load(texturePath.c_str(), &width, &height, &channels, STBI_rgb_alpha);

                VkDeviceSize vkTextureSize = width * height * 4; // each pixel takes up 4 bytes: 1
                                                                 // for R, G, B, A each.

                if (pixels == nullptr) {
                        FATAL("Failed to load texture {}", texturePath);
                }

                // create staging buffer and copy over the pixels
                VkBuffer stagingBuffer;
                VkDeviceMemory stagingBufferMemory;

                VulkanUtils::createBuffer(
                        _physicalDevice,
                        _logicalDevice,
                        vkTextureSize,
                        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                        stagingBuffer,
                        stagingBufferMemory
                );

                VulkanUtils::vkMemCopy(pixels, stagingBufferMemory, vkTextureSize, _logicalDevice);

                stbi_image_free(pixels);

                // create image object
                VkImage textureImage;
                VkDeviceMemory textureImageMemory;

                createImage(
                        width,
                        height,
                        VK_FORMAT_R8G8B8A8_SRGB,
                        VK_IMAGE_TILING_OPTIMAL,
                        VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                        textureImage,
                        textureImageMemory
                );

                transitionImageLayout(textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
                copyBufferToImage(stagingBuffer, textureImage, static_cast<uint32_t>(width), static_cast<uint32_t>(height));
                // transition again for shader read
                transitionImageLayout(textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

                // clean up staging buffer
                vkDestroyBuffer(_logicalDevice, stagingBuffer, nullptr);
                vkFreeMemory(_logicalDevice, stagingBufferMemory, nullptr);

                _textures.emplace(std::make_pair(texturePath, Texture{textureImage, textureImageMemory}));
        }

      private:
        struct Texture {
                VkImage textureImage;
                VkDeviceMemory textureImageMemory; // gpu memory that holds the image.
        };

        // creates an empty image and allocate device memory for it.
        void createImage(
                uint32_t width,
                uint32_t height,
                VkFormat format,
                VkImageTiling tiling,
                VkImageUsageFlags usage,
                VkMemoryPropertyFlags properties,
                VkImage& image,
                VkDeviceMemory& imageMemory
        ) {
                VkImageCreateInfo imageInfo{};
                imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
                imageInfo.imageType = VK_IMAGE_TYPE_2D;
                imageInfo.extent.width = width;
                imageInfo.extent.height = height;
                imageInfo.extent.depth = 1;
                imageInfo.mipLevels = 1;
                imageInfo.arrayLayers = 1;
                imageInfo.format = format;
                imageInfo.tiling = tiling;
                imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
                imageInfo.usage = usage;
                imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
                imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

                if (vkCreateImage(_logicalDevice, &imageInfo, nullptr, &image) != VK_SUCCESS) {
                        throw std::runtime_error("failed to create image!");
                }

                VkMemoryRequirements memRequirements;
                vkGetImageMemoryRequirements(_logicalDevice, image, &memRequirements);

                VkMemoryAllocateInfo allocInfo{};
                allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
                allocInfo.allocationSize = memRequirements.size;
                allocInfo.memoryTypeIndex
                        = VulkanUtils::findMemoryType(_physicalDevice, memRequirements.memoryTypeBits, properties);

                if (vkAllocateMemory(_logicalDevice, &allocInfo, nullptr, &imageMemory) != VK_SUCCESS) {
                        FATAL("Failed to allocate image memory!");
                }

                vkBindImageMemory(_logicalDevice, image, imageMemory, 0);
        }

        void transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout) {
                VkCommandBuffer commandBuffer = VulkanUtils::beginSingleTimeCommands(_logicalDevice, _commandPool);

                // create a barrier to transition layout
                VkImageMemoryBarrier barrier{};
                barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
                barrier.oldLayout = oldLayout;
                barrier.newLayout = newLayout;

                // set queue families to ignored because we're not transferring
                // queue family ownership(another usage of the barrier)
                barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

                barrier.image = image;
                barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                barrier.subresourceRange.baseMipLevel = 0;
                barrier.subresourceRange.levelCount = 1;
                barrier.subresourceRange.baseArrayLayer = 0;
                barrier.subresourceRange.layerCount = 1;

                VkPipelineStageFlags sourceStage;
                VkPipelineStageFlags destinationStage;

                // before texture from staging buffer
                if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
                        barrier.srcAccessMask = 0;
                        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

                        sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
                        destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
                } else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) { // after texture transfer
                        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
                        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

                        sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
                        destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
                } else {
                        FATAL("Unsupported texture layout transition!");
                }

                vkCmdPipelineBarrier(
                        commandBuffer, sourceStage, destinationStage, 0, 0, nullptr, 0, nullptr, 1, &barrier
                );

                VulkanUtils::endSingleTimeCommands(commandBuffer, _logicalDevice, _graphicsQueue, _commandPool);
        }

        // copy over content  in the staging buffer to the actual image
        void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height) {
                VkCommandBuffer commandBuffer = VulkanUtils::beginSingleTimeCommands(_logicalDevice, _commandPool);

                VkBufferImageCopy region{};
                region.bufferOffset = 0;
                // in some cases the pixels aren't tightly packed, specify them.
                region.bufferRowLength = 0;
                region.bufferImageHeight = 0;

                region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                region.imageSubresource.mipLevel = 0;
                region.imageSubresource.baseArrayLayer = 0;
                region.imageSubresource.layerCount = 1;

                region.imageOffset = {0, 0, 0};
                region.imageExtent = {width, height, 1};

                vkCmdCopyBufferToImage(commandBuffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
                VulkanUtils::endSingleTimeCommands(commandBuffer, _logicalDevice, _graphicsQueue, _commandPool);
        }

        std::unordered_map<std::string, Texture> _textures; // image path -> texture obj
        VkPhysicalDevice _physicalDevice;
        VkDevice _logicalDevice;
        VkCommandPool _commandPool;
        VkQueue _graphicsQueue;
};