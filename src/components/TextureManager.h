#pragma once
#include "components/VulkanUtils.h"
#include <cstring>
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
        );

        ~TextureManager();

        // clean up and deallocate everything.
        // suggested to call before cleaning up swapchain.
        void Cleanup();

        void GetDescriptorImageInfo(const std::string& texturePath, VkDescriptorImageInfo& imageInfo);

        void LoadTexture(const std::string& texturePath);

      private:
        struct Texture {
                VkImage textureImage;
                VkImageView textureImageView;
                VkDeviceMemory textureImageMemory; // gpu memory that holds the image.
                VkSampler textureSampler; // sampler for shaders
        };

        void transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout);

        // copy over content  in the staging buffer to the actual image
        void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);

        std::unordered_map<std::string, Texture> _textures; // image path -> texture obj
        VkPhysicalDevice _physicalDevice;
        VkDevice _logicalDevice;
        VkCommandPool _commandPool;
        VkQueue _graphicsQueue;
};