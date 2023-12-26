#include "vulkan/vulkan.h"
#include <optional>
#include <vulkan/vulkan_core.h>

struct QueueFamilyIndices {
        std::optional<uint32_t> graphicsFamily;
        std::optional<uint32_t> presentationFamily;
        inline bool isComplete() { return graphicsFamily.has_value() && presentationFamily.has_value(); }
};

/**
 * @brief Vulkan device representation.Call InitQueueFamilyIndices() with the right surface, then call
 * CreateLogicalDeviceAndQueue() with the right extensions.
 *
 */
struct VQDevice {
        /** @brief Physical device representation */
        VkPhysicalDevice physicalDevice;
        /** @brief Logical device representation (application's view of the device) */
        VkDevice logicalDevice;
        /** @brief Properties of the physical device including limits that the application can check against */
        VkPhysicalDeviceProperties properties;
        /** @brief Features of the physical device that an application can use to check if a feature is supported */
        VkPhysicalDeviceFeatures features;
        /** @brief Features that have been enabled for use on the physical device */
        VkPhysicalDeviceFeatures enabledFeatures;
        /** @brief Memory types and heaps of the physical device */
        VkPhysicalDeviceMemoryProperties memoryProperties;
        /** @brief Queue family properties of the physical device */
        std::vector<VkQueueFamilyProperties> queueFamilyProperties;
        /** @brief List of extensions supported by the device */
        std::vector<std::string> supportedExtensions;

        VkQueue graphicsQueue = VK_NULL_HANDLE;

        VkQueue presentationQueue = VK_NULL_HANDLE;

        VkCommandPool graphicsCommandPool = VK_NULL_HANDLE;

        /** @brief Contains queue family indices */
        QueueFamilyIndices queueFamilyIndices;
        operator VkDevice() const { return logicalDevice; };
        explicit VQDevice(VkPhysicalDevice physicalDevice);
        ~VQDevice();

        /**
         * @brief Query Vulkan API to find the queue family indices that support graphics and presentation.
         * 
         * @param surface The surface on which the presentation queue will present to.
         */
        void InitQueueFamilyIndices(VkSurfaceKHR surface);

        /**
         * @brief Create a Logical Device, and create a graphics queue and a presentation queue.
         * 
         * @param extensions the extensions to enable
         */
        void CreateLogicalDeviceAndQueue(const std::vector<const char*>& extensions);

        /**
         * @brief Create a Graphics Command Pool, the pool is used for allocating command buffers.
         */
        void CreateGraphicsCommandPool();

        /**
         * @brief Populate a vector of VkCommandBuffer with the given command buffer count. The command buffers can be
         * used to submit to the graphics command queue.
         * Note that the caller is responsible for freeing the command buffers.
         *
         * @param commandBuffers the vector to populate
         * @param commandBufferCount    the number of command buffers to create
         */
        void CreateGraphicsCommandBuffer(std::vector<VkCommandBuffer>& commandBuffers, uint32_t commandBufferCount);

        void FreeGraphicsCommandBuffer(const std::vector<VkCommandBuffer>& commandBuffers);

        void Cleanup();
};