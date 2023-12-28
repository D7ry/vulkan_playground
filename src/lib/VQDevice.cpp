#include <cstdint>
#include <set>
#include <vulkan/vulkan_core.h>
#include "VQDevice.h"
#include "VQBuffer.h"
#include "VQUtils.h"
void VQDevice::CreateLogicalDeviceAndQueue(const std::vector<const char*>& extensions) {
        if (!this->queueFamilyIndices.isComplete()) {
                FATAL("Queue family indices incomplete! Call InitQueueFamilyIndices().");
        }
        std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
        std::set<uint32_t> uniqueQueueFamilyIndices;
        uniqueQueueFamilyIndices.insert(this->queueFamilyIndices.graphicsFamily.value());
        uniqueQueueFamilyIndices.insert(this->queueFamilyIndices.presentationFamily.value());

        INFO("Found {} unique queue families.", uniqueQueueFamilyIndices.size());

        VkPhysicalDeviceFeatures deviceFeatures{}; // no features for no
        VkDeviceCreateInfo createInfo{};
        float queuePriority = 1.f;
        for (uint32_t queueFamily : uniqueQueueFamilyIndices) {
                VkDeviceQueueCreateInfo queueCreateInfo{};
                queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
                queueCreateInfo.queueFamilyIndex = queueFamily;
                queueCreateInfo.queueCount = 1;
                queueCreateInfo.pQueuePriorities = &queuePriority;
                queueCreateInfos.push_back(queueCreateInfo);
        }
        // populate createInfo
        createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
        createInfo.pQueueCreateInfos = queueCreateInfos.data();
        createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        createInfo.pEnabledFeatures = &deviceFeatures;
        createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
        createInfo.ppEnabledExtensionNames = extensions.data(); // enable swapchain extension
        if (vkCreateDevice(this->physicalDevice, &createInfo, nullptr, &this->logicalDevice) != VK_SUCCESS) {
                FATAL("Failed to create logical device!");
        }
        vkGetDeviceQueue(this->logicalDevice, queueFamilyIndices.graphicsFamily.value(), 0, &this->graphicsQueue);
        vkGetDeviceQueue(this->logicalDevice, queueFamilyIndices.presentationFamily.value(), 0, &this->presentationQueue);
}
void VQDevice::InitQueueFamilyIndices(VkSurfaceKHR surface) {
        INFO("Finding graphics and presentation queue families...");
        uint32_t queueFamilyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, nullptr);
        std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount
        ); // initialize vector to store queue familieis
        vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, queueFamilies.data());
        int i = 0;
        for (const auto& queueFamily : queueFamilies) {
                VkBool32 presentationSupport = false;
                if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
                        this->queueFamilyIndices.graphicsFamily = i;
                        INFO("Graphics family found at {}", i);
                }
                vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, i, surface, &presentationSupport);
                if (presentationSupport) {
                        this->queueFamilyIndices.presentationFamily = i;
                        INFO("Presentation family found at {}", i);
                }
                if (this->queueFamilyIndices.isComplete()) {
                        break;
                }
                i++;
        }
}
void VQDevice::CreateGraphicsCommandPool() {
        if (!queueFamilyIndices.graphicsFamily.has_value()) {
                FATAL("Graphics queue family not initialized! Call InitQueueFamilyIndices().");
        }
        VkCommandPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT; // ALlow command buffers to be re-recorded individually
        // we want to re-record the command buffer every single frame.
        poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();

        if (vkCreateCommandPool(this->logicalDevice, &poolInfo, nullptr, &this->graphicsCommandPool) != VK_SUCCESS) {
                FATAL("Failed to create command pool!");
        }
}
void VQDevice::CreateGraphicsCommandBuffer(std::vector<VkCommandBuffer>& commandBuffers, uint32_t commandBufferCount) {
        commandBuffers.resize(commandBufferCount);

        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.commandPool = graphicsCommandPool;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandBufferCount = (uint32_t)commandBuffers.size();

        if (vkAllocateCommandBuffers(logicalDevice, &allocInfo, commandBuffers.data()) != VK_SUCCESS) {
                FATAL("Failed to allocate command buffers!");
        }
}
VQDevice::VQDevice(VkPhysicalDevice physicalDevice) {
        this->physicalDevice = physicalDevice;
        // Store Properties features, limits and properties of the physical device for later use
        // Device properties also contain limits and sparse properties
        vkGetPhysicalDeviceProperties(physicalDevice, &properties);
        // Features should be checked by the examples before using them
        vkGetPhysicalDeviceFeatures(physicalDevice, &features);
        // Memory properties are used regularly for creating all kinds of buffers
        vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memoryProperties);
        // Queue family properties, used for setting up requested queues upon device creation
        uint32_t queueFamilyCount;
        vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, nullptr);
        assert(queueFamilyCount > 0);
        queueFamilyProperties.resize(queueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, queueFamilyProperties.data());

        // Get list of supported extensions
        uint32_t extCount = 0;
        vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extCount, nullptr);
        if (extCount > 0) {
                std::vector<VkExtensionProperties> extensions(extCount);
                if (vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extCount, &extensions.front())
                    == VK_SUCCESS) {
                        for (auto ext : extensions) {
                                supportedExtensions.push_back(ext.extensionName);
                        }
                }
        }
}
void VQDevice::FreeGraphicsCommandBuffer(const std::vector<VkCommandBuffer>& commandBuffers) {
        vkFreeCommandBuffers(logicalDevice, graphicsCommandPool, commandBuffers.size(), commandBuffers.data());
}
VQBuffer VQDevice::CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties) {
        VQBuffer vqBuffer{};

        vqBuffer.device = this->logicalDevice;
        vqBuffer.size = size;

        VkBufferCreateInfo bufferInfo{};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.size = size;
        bufferInfo.usage = usage;
        bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        if (vkCreateBuffer(this->logicalDevice, &bufferInfo, nullptr, &vqBuffer.buffer) != VK_SUCCESS) {
                FATAL("Failed to create VK buffer!");
        }

        VkMemoryRequirements memRequirements;
        vkGetBufferMemoryRequirements(this->logicalDevice, vqBuffer.buffer, &memRequirements);

        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex
                = VQUtils::findMemoryType(physicalDevice, memRequirements.memoryTypeBits, properties);

        if (vkAllocateMemory(this->logicalDevice, &allocInfo, nullptr, &vqBuffer.bufferMemory) != VK_SUCCESS) {
                FATAL("Failed to allocate device memory for buffer creation!");
        }

        vkBindBufferMemory(this->logicalDevice, vqBuffer.buffer, vqBuffer.bufferMemory, 0);

        vkMapMemory(this->logicalDevice, vqBuffer.bufferMemory, 0, vqBuffer.size, 0, &vqBuffer.bufferAddress);

        return vqBuffer;
}
VQDevice::~VQDevice() {

}
void VQDevice::Cleanup() {        
        if (graphicsCommandPool != VK_NULL_HANDLE) {
                vkDestroyCommandPool(logicalDevice, graphicsCommandPool, nullptr);
        }
        vkDestroyDevice(logicalDevice, nullptr);
}
