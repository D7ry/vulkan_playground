#include "VulkanUtils.h"
#include <vulkan/vulkan_core.h>
VkCommandBuffer VulkanUtils::beginSingleTimeCommands(VkDevice device, VkCommandPool commandPool) {
        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandPool = commandPool;
        allocInfo.commandBufferCount = 1;

        VkCommandBuffer commandBuffer;
        vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer);

        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

        vkBeginCommandBuffer(commandBuffer, &beginInfo);

        return commandBuffer;
}
void VulkanUtils::endSingleTimeCommands(VkCommandBuffer commandBuffer, VkDevice device, VkQueue graphicsQueue, VkCommandPool commandPool) {
        vkEndCommandBuffer(commandBuffer);

        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffer;

        vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
        vkQueueWaitIdle(graphicsQueue);

        vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
}
void VulkanUtils::createCommandBuffers(VkCommandBuffer* commandBuffer, uint32_t commandBufferCount, VkCommandPool commandPool, VkDevice device) {
        VkCommandBufferAllocateInfo commandBufferAllocateInfo = {};
        commandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        commandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        commandBufferAllocateInfo.commandPool = commandPool;
        commandBufferAllocateInfo.commandBufferCount = commandBufferCount;
        if (vkAllocateCommandBuffers(device, &commandBufferAllocateInfo, commandBuffer) != VK_SUCCESS) {
                FATAL("Could not allocate command buffers");
        }
}
void VulkanUtils::createCommandBuffers(
        std::vector<VkCommandBuffer>& commandBuffers,
        uint32_t commandBufferCount,
        VkCommandPool commandPool,
        VkDevice device
) {
        commandBuffers.resize(commandBufferCount);
        createCommandBuffers(commandBuffers.data(), commandBufferCount, commandPool, device);
}
void VulkanUtils::createCommandPool(VkCommandPool* commandPool, VkCommandPoolCreateFlags flags, uint32_t queueFamilyIndex, VkDevice device) {
        VkCommandPoolCreateInfo commandPoolCreateInfo = {};
        commandPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        commandPoolCreateInfo.queueFamilyIndex = queueFamilyIndex;
        commandPoolCreateInfo.flags = flags;

        if (vkCreateCommandPool(device, &commandPoolCreateInfo, nullptr, commandPool) != VK_SUCCESS) {
                FATAL("Could not create graphics command pool");
        }
}
void VulkanUtils::createCommandPoolAndBuffers(
        VkCommandPool& commandPool,
        VkCommandBuffer& commandBuffer,
        uint32_t commandBufferCount,
        VkCommandPoolCreateFlags flags,
        uint32_t queueFamilyIndex,
        VkDevice device
) {
        createCommandPool(&commandPool, flags, queueFamilyIndex, device);
        createCommandBuffers(&commandBuffer, commandBufferCount, commandPool, device);
}

uint32_t VulkanUtils::findMemoryType(VkPhysicalDevice physicalDevice, uint32_t typeFilter, VkMemoryPropertyFlags properties) {
        VkPhysicalDeviceMemoryProperties memProperties;
        vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

        for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
                if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
                        return i;
                }
        }

        FATAL("Failed to find suitable memory type!");
}
void VulkanUtils::createBuffer(
        VkPhysicalDevice physicalDevice,
        VkDevice device,
        VkDeviceSize size,
        VkBufferUsageFlags usage,
        VkMemoryPropertyFlags properties,
        VkBuffer& buffer,
        VkDeviceMemory& bufferMemory
) {
        VkBufferCreateInfo bufferInfo{};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.size = size;
        bufferInfo.usage = usage;
        bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        if (vkCreateBuffer(device, &bufferInfo, nullptr, &buffer) != VK_SUCCESS) {
                FATAL("Failed to create VK buffer!");
        }

        VkMemoryRequirements memRequirements;
        vkGetBufferMemoryRequirements(device, buffer, &memRequirements);

        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = findMemoryType(physicalDevice, memRequirements.memoryTypeBits, properties);

        if (vkAllocateMemory(device, &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS) {
                FATAL("Failed to allocate device memory for buffer creation!");
        }

        vkBindBufferMemory(device, buffer, bufferMemory, 0);
}
void VulkanUtils::vkMemCopy(void* src, VkDeviceMemory dstMemory, VkDeviceSize size, VkDevice dstDevice) {
        void* data;
        vkMapMemory(dstDevice, dstMemory, 0, size, 0, &data);
        memcpy(data, src, static_cast<size_t>(size));
        vkUnmapMemory(dstDevice, dstMemory);
}

void VulkanUtils::copyBuffer(VkDevice device, VkCommandPool commandPool, VkQueue queue, VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size) {
        VkCommandBuffer commandBuffer = beginSingleTimeCommands(device, commandPool);

        VkBufferCopy copyRegion{};
        copyRegion.size = size;
        vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

        endSingleTimeCommands(commandBuffer, device, queue, commandPool);
}

VkImageView VulkanUtils::createImageView(VkImage& textureImage, VkDevice& logicalDevice, VkFormat format, VkImageAspectFlags flags) {
        VkImageViewCreateInfo viewInfo{};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = textureImage;
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = format;
        viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = 1;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = 1;
        viewInfo.subresourceRange.aspectMask = flags;

        VkImageView imageView;
        if (vkCreateImageView(logicalDevice, &viewInfo, nullptr, &imageView) != VK_SUCCESS) {
                FATAL("Failed to create texture image view!");
        }

        return imageView;
}
void VulkanUtils::createImage(
        uint32_t width,
        uint32_t height,
        VkFormat format,
        VkImageTiling tiling,
        VkImageUsageFlags usage,
        VkMemoryPropertyFlags properties,
        VkImage& image,
        VkDeviceMemory& imageMemory,
        VkPhysicalDevice physicalDevice,
        VkDevice logicalDevice
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

        if (vkCreateImage(logicalDevice, &imageInfo, nullptr, &image) != VK_SUCCESS) {
                throw std::runtime_error("failed to create image!");
        }

        VkMemoryRequirements memRequirements;
        vkGetImageMemoryRequirements(logicalDevice, image, &memRequirements);

        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex
                = VulkanUtils::findMemoryType(physicalDevice, memRequirements.memoryTypeBits, properties);

        if (vkAllocateMemory(logicalDevice, &allocInfo, nullptr, &imageMemory) != VK_SUCCESS) {
                FATAL("Failed to allocate image memory!");
        }

        vkBindImageMemory(logicalDevice, image, imageMemory, 0);
}
VkFormat VulkanUtils::findDepthFormat(VkPhysicalDevice physicalDevice) {
        return findBestFormat(
                {VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT},
                VK_IMAGE_TILING_OPTIMAL,
                VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT,
                physicalDevice
        );
}
VkFormat VulkanUtils::findBestFormat(
        const std::vector<VkFormat>& candidates,
        VkImageTiling tiling,
        VkFormatFeatureFlags features,
        VkPhysicalDevice physicalDevice
) {
        for (VkFormat format : candidates) {
                VkFormatProperties formatProperties;
                vkGetPhysicalDeviceFormatProperties(physicalDevice, format, &formatProperties);
                if (tiling == VK_IMAGE_TILING_LINEAR
                    && (formatProperties.linearTilingFeatures & features) == features) {
                        return format;
                } else if (tiling == VK_IMAGE_TILING_OPTIMAL && (formatProperties.optimalTilingFeatures & features) == features) {
                        return format;
                }
        }
        FATAL("Failed tot find format!");
        return VK_FORMAT_R8G8B8A8_SRGB; // unreacheable
};
std::pair<VkBuffer, VkDeviceMemory>
VulkanUtils::createStagingBuffer(VkPhysicalDevice physicalDevice, VkDevice device, VkDeviceSize bufferSize) {
        VkBuffer stagingBuffer;
        VkDeviceMemory stagingBufferMemory;
        VulkanUtils::createBuffer(
                physicalDevice,
                device,
                bufferSize,
                VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                stagingBuffer,
                stagingBufferMemory
        );
        return {stagingBuffer, stagingBufferMemory};
}
void VulkanUtils::createVertexBuffer(
        VkPhysicalDevice physicalDevice,
        VkDevice device,
        std::vector<Vertex>& vertices,
        MetaBuffer& metaBuffer,
        VkCommandPool pool,
        VkQueue queue
) {
        VkDeviceSize vertexBufferSize = sizeof(vertices[0]) * vertices.size();

        std::pair<VkBuffer, VkDeviceMemory> res = createStagingBuffer(physicalDevice, device, vertexBufferSize);
        VkBuffer stagingBuffer = res.first;
        VkDeviceMemory stagingBufferMemory = res.second;
        // copy over data from cpu memory to gpu memory(staging buffer)
        void* data;
        vkMapMemory(device, stagingBufferMemory, 0, vertexBufferSize, 0, &data);
        memcpy(data, vertices.data(), (size_t)vertexBufferSize); // copy the data
        vkUnmapMemory(device, stagingBufferMemory);

        // create vertex buffer
        VulkanUtils::createBuffer(
                physicalDevice,
                device,
                vertexBufferSize,
                VK_BUFFER_USAGE_TRANSFER_DST_BIT // can be used as destination in a memory transfer operation
                        | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, // local to the GPU for faster access
                metaBuffer.buffer,
                metaBuffer.bufferMemory
        );

        VulkanUtils::copyBuffer(device, pool, queue, stagingBuffer, metaBuffer.buffer, vertexBufferSize);

        metaBuffer.size = vertexBufferSize;

        // get rid of staging buffer, it is very much temproary
        vkDestroyBuffer(device, stagingBuffer, nullptr);
        vkFreeMemory(device, stagingBufferMemory, nullptr);
}
void VulkanUtils::createIndexBuffer(
        MetaBuffer& metaBuffer,
        std::vector<uint32_t> indices,
        VkDevice device,
        VkPhysicalDevice physicalDevice,
        VkCommandPool pool,
        VkQueue queue
) {
        INFO("Creating index buffer...");
        VkDeviceSize indexBufferSize = sizeof(indices[0]) * indices.size();

        std::pair<VkBuffer, VkDeviceMemory> res = createStagingBuffer(physicalDevice, device, indexBufferSize);
        VkBuffer stagingBuffer = res.first;
        VkDeviceMemory stagingBufferMemory = res.second;
        // copy over data from cpu memory to gpu memory(staging buffer)
        void* data;
        vkMapMemory(device, stagingBufferMemory, 0, indexBufferSize, 0, &data);
        memcpy(data, indices.data(), (size_t)indexBufferSize); // copy the data
        vkUnmapMemory(device, stagingBufferMemory);

        // create index buffer
        VulkanUtils::createBuffer(
                physicalDevice,
                device,
                indexBufferSize,
                VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                metaBuffer.buffer,
                metaBuffer.bufferMemory
        );

        VulkanUtils::copyBuffer(device, pool, queue, stagingBuffer, metaBuffer.buffer, indexBufferSize);

        metaBuffer.size = indexBufferSize;

        vkDestroyBuffer(device, stagingBuffer, nullptr);
        vkFreeMemory(device, stagingBufferMemory, nullptr);
}
