#pragma once
#include <cstdint>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <optional>

#include <vector>
#include "components/DeltaTimer.h"
#include "components/ImGuiManager.h"
// TODO: create a serialization scheme for tweakable settings.

/**
 * @brief A Vulkan application.
 *  Vulkan applications are self-contained programs that has its own window,
 * logical device, and rendering pipeline. They are responsible for initializing
 * Vulkan, creating a window, and entering the main render loop. To create a
 * Vulkan application, create a class that inherits from this class and
 * implement the Run() method.
 */
class VulkanApplication {
      public:
        void Run();

      protected:
        struct QueueFamilyIndices {
                std::optional<uint32_t> graphicsFamily;
                std::optional<uint32_t> presentationFamily;
                inline bool isComplete() { return graphicsFamily.has_value() && presentationFamily.has_value(); }
        };

        struct SwapChainSupportDetails {
                VkSurfaceCapabilitiesKHR capabilities;
                std::vector<VkSurfaceFormatKHR> formats;
                std::vector<VkPresentModeKHR> presentModes;
        };

        /**
         * @brief Initializes a GLFW window and set it as a member variable for
         * drawing.
         */
        void initWindow();
        /**
         * @brief Creates a vulkan instance, save to a field of this class.
         */
        void createInstance();

        void createSurface();

        QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device);

        bool checkDeviceExtensionSupport(VkPhysicalDevice device);

        bool isDeviceSuitable(VkPhysicalDevice device);

        void pickPhysicalDevice();

        void createLogicalDevice();

        SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device);

        VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);

        VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);

        VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);

        void createSwapChain();
        void recreateSwapChain();
        void cleanupSwapChain();

        void createImageViews();

        virtual void postInit();
        virtual void createDescriptorPool();
        /**
         * @brief Initialize and configure descriptor sets.
         * Before calling this, a descripto pool must be created, as well as a descriptor set layout.
         */
        virtual void createDescriptorSets();
        virtual void createVertexBuffer();
        virtual void createIndexBuffer();
        virtual void createUniformBuffers();
        virtual void createDescriptorSetLayout();
        virtual void createGraphicsPipeline();
        virtual void createRenderPass();
        virtual void createFramebuffers();
        virtual void createCommandPool();
        virtual void createCommandBuffer();
        virtual void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);

        void setKeyCallback();
        void setCursorPosCallback();
        /**
         * @brief Override this method to handle key events.
         */
        virtual  void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
        virtual void cursorPosCallback(GLFWwindow* window, double xpos, double ypos);

        virtual void renderImGui();

        virtual void updateUniformBufferData(uint32_t frameIndex);


        void createSynchronizationObjects();

        /**
         * @brief Initializes Vulkan.
         *
         */
        void initVulkan();
        /**
         * @brief Enter the main render loop.
         *
         */
        void mainLoop();
        virtual void drawFrame();

        void cleanup();

        // utility methods

        /**
         * @brief FInd the memory type that conforms to typeFilter and properties.
         * TypeFilter are obtained form vkGetBufferMemoryRequirements().memoryTypeBits
         */
        static uint32_t findMemoryType(VkPhysicalDevice physicalDevice, uint32_t typeFilter, VkMemoryPropertyFlags properties);
        /**
         * @brief Allocate buffer on both CPU and GPU memory, and bind them together.
         */
        static void createBuffer(
                VkPhysicalDevice physicalDevice,
                VkDevice device,
                VkDeviceSize size,
                VkBufferUsageFlags usage,
                VkMemoryPropertyFlags properties,
                VkBuffer& buffer,
                VkDeviceMemory& bufferMemory
        );

        static std::pair<VkBuffer, VkDeviceMemory> createStagingBuffer(VulkanApplication* app, VkDeviceSize bufferSize);


        // validation layers, enable under debug mode only
#ifdef NDEBUG
        const bool enableValidationLayers = false;
#else
        const bool enableValidationLayers = true;
#endif
        static inline const std::vector<const char*> VALIDATION_LAYERS = {"VK_LAYER_KHRONOS_validation"};
        static inline const std::vector<const char*> DEVICE_EXTENSIONS = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};
        bool checkValidationLayerSupport();

        // debug messenger setup
        static VkResult CreateDebugUtilsMessengerEXT(
                VkInstance instance,
                const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
                const VkAllocationCallbacks* pAllocator,
                VkDebugUtilsMessengerEXT* pDebugMessenger
        );
        void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);
        void setupDebugMessenger();
        static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
                VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                VkDebugUtilsMessageTypeFlagsEXT messageType,
                const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
                void* pUserData
        );

        // private fields
        static inline const char* APPLICATION_NAME = "Vulkan Application";
        static inline const uint32_t APPLICATION_VERSION = VK_MAKE_VERSION(1, 0, 0);
        static inline const char* ENGINE_NAME = "No Engine";
        static inline const uint32_t ENGINE_VERSION = VK_MAKE_VERSION(1, 0, 0);
        static inline const uint32_t API_VERSION = VK_API_VERSION_1_0;
        static inline const int MAX_FRAMES_IN_FLIGHT = 2;

        static void framebufferResizeCallback(GLFWwindow* window, int width, int height);

        GLFWwindow* _window;
        VkInstance _instance;
        VkSurfaceKHR _surface;
        VkDebugUtilsMessengerEXT _debugMessenger;

        // devices
        VkPhysicalDevice _physicalDevice = VK_NULL_HANDLE;
        VkDevice _logicalDevice = VK_NULL_HANDLE;

        VkQueue _graphicsQueue = VK_NULL_HANDLE;
        VkQueue _presentationQueue = VK_NULL_HANDLE;

        // swapchain
        VkSwapchainKHR _swapChain = VK_NULL_HANDLE;
        std::vector<VkImage> _swapChainImages;
        VkFormat _swapChainImageFormat;
        VkExtent2D _swapChainExtent; // resolution of the swapchain images

        std::vector<VkFramebuffer> _swapChainFrameBuffers;

        std::vector<VkImageView> _swapChainImageViews;

        VkDescriptorSetLayout _descriptorSetLayout = VK_NULL_HANDLE;

        VkPipelineLayout _pipelineLayout = VK_NULL_HANDLE;
        VkRenderPass _renderPass = VK_NULL_HANDLE;

        VkPipeline _graphicsPipeline = VK_NULL_HANDLE;

        VkCommandPool _commandPool = VK_NULL_HANDLE;
        
        VkDescriptorPool _descriptorPool = VK_NULL_HANDLE;
        std::vector<VkDescriptorSet> _descriptorSets;

        VkBuffer _vertexBuffer = VK_NULL_HANDLE;
        VkDeviceMemory _vertexBufferMemory = VK_NULL_HANDLE;

        VkBuffer _indexBuffer = VK_NULL_HANDLE;
        VkDeviceMemory _indexBufferMemory = VK_NULL_HANDLE;

        // Uniform buffers are stored in vectors because they are per-frame.
        // for each in flight frame, there is a uniform buffer.
        std::vector<VkBuffer> _uniformBuffers;
        std::vector<VkDeviceMemory> _uniformBuffersMemory;
        std::vector<void*> _uniformBuffersData;

        std::vector<VkCommandBuffer> _commandBuffers;
        std::vector<VkSemaphore> _semaImageAvailable;
        std::vector<VkSemaphore> _semaRenderFinished;
        std::vector<VkFence> _fenceInFlight;

        bool _framebufferResized = false;

        uint32_t _currentFrame = 0;

        DeltaTimer _deltaTimer;

        ImGuiManager _imguiManager;

};
