#pragma once
#include <cstdint>
#include <memory>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <optional>

#include <vector>
#include "components/DeltaTimer.h"
#include "components/ImGuiManager.h"
#include "lib/VQDevice.h"
#include "components/InputManager.h"
#include "components/Camera.h"
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

        VkPhysicalDevice pickPhysicalDevice();

        //void createLogicalDevice();

        SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device);

        VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);

        VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);

        VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);

        void createSwapChain();
        void recreateSwapChain();
        void cleanupSwapChain();

        void createImageViews();

        virtual void middleInit();
        virtual void postInit();
        virtual void createDepthBuffer();
        virtual void createUniformBuffers();
        virtual void createRenderPass();
        virtual void createFramebuffers();
        // virtual void createCommandPool();
        // virtual void createCommandBuffer();
        virtual void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);

        //void loadModel();

        void setKeyCallback();
        void setCursorInputCallback();
        /**
         * @brief Override this method to handle key events.
         */
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
        virtual void preCleanup() {};
        virtual void postCleanup() {};

        // utility methods

        /**
         * @brief FInd the memory type that conforms to typeFilter and properties.
         * TypeFilter are obtained form vkGetBufferMemoryRequirements().memoryTypeBits
         */
        static uint32_t findMemoryType(VkPhysicalDevice physicalDevice, uint32_t typeFilter, VkMemoryPropertyFlags properties);

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

        // swapchain
        VkSwapchainKHR _swapChain = VK_NULL_HANDLE;
        std::vector<VkImage> _swapChainImages;
        VkFormat _swapChainImageFormat;
        VkExtent2D _swapChainExtent; // resolution of the swapchain images

        std::vector<VkFramebuffer> _swapChainFrameBuffers;

        std::vector<VkImageView> _swapChainImageViews;

        VkRenderPass _renderPass = VK_NULL_HANDLE;

        std::vector<VkCommandBuffer> _commandBuffers;
        std::vector<VkSemaphore> _semaImageAvailable;
        std::vector<VkSemaphore> _semaRenderFinished;
        std::vector<VkFence> _fenceInFlight;

        // depth buffer
        VkImage _depthImage;
        VkDeviceMemory _depthImageMemory;
        VkImageView _depthImageView;

        bool _framebufferResized = false;

        uint32_t _currentFrame = 0;

        DeltaTimer _deltaTimer;

        ImGuiManager _imguiManager;

        std::shared_ptr<VQDevice> _device;

        std::unique_ptr<InputManager> _inputManager;

        Camera _camera = Camera(glm::vec3(1.0f, 0.5f, 1.0f), glm::vec3(-28, -146, 0));
        bool viewMode = false;
};
