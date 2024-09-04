#pragma once
#include "components/Camera.h"
#include "components/DeltaTimer.h"
#include "components/InputManager.h"
#include "ecs/system/EntityViewerSystem.h"
#include <cstdint>
#include <glm/detail/qualifier.hpp>
#include <imgui_internal.h>
#include <memory>
#include <optional>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>
#if __APPLE__
#include <vulkan/vulkan_beta.h> // VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME
#endif

#include "structs/TickData.h"

#include "components/ImGuiManager.h"
#include "imgui.h"
#include "lib/VQDevice.h"
#include <vector>

#include <filesystem>

#include "lib/DeletionStack.h"

#include "ecs/system/PhongRenderSystem.h"

class TickData;


// TODO: create a serialization scheme for tweakable settings.

/**
 * @brief A render manager responsible for rendering everything
 */
class VulkanEngine
{
  public:
    /**
     * @brief Initialize render manager, create necessary vulkan resources and
     * bindings.
     */
    void Init();
    void Run();
    void Tick();
    void Cleanup();

    void DrawImgui();

    void SetImguiRenderCallback(std::function<void()> imguiFunction);

    inline std::pair<uint32_t, uint32_t> GetSwapchainExtent() {
        return {_swapChainExtent.width, _swapChainExtent.height};
    }

  protected:
    struct QueueFamilyIndices
    {
        std::optional<uint32_t> graphicsFamily;
        std::optional<uint32_t> presentationFamily;

        inline bool isComplete() {
            return graphicsFamily.has_value() && presentationFamily.has_value();
        }
    };

    struct SwapChainSupportDetails
    {
        VkSurfaceCapabilitiesKHR capabilities;
        std::vector<VkSurfaceFormatKHR> formats;
        std::vector<VkPresentModeKHR> presentModes;
    };

    void initGLFW();

    /**
     * @brief Creates a vulkan instance, save to a field of this class.
     */
    void createInstance();

    void createSurface();

    QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device);

    bool checkDeviceExtensionSupport(VkPhysicalDevice device);

    bool isDeviceSuitable(VkPhysicalDevice device);

    VkPhysicalDevice pickPhysicalDevice();

    // void createLogicalDevice();

    SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device);

    VkSurfaceFormatKHR chooseSwapSurfaceFormat(
        const std::vector<VkSurfaceFormatKHR>& availableFormats
    );

    VkPresentModeKHR chooseSwapPresentMode(
        const std::vector<VkPresentModeKHR>& availablePresentModes
    );

    VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);

    /** @brief Internally calls createSwapChain, but also pushes a swapchain
     * cleanup function once, to clean up the last swapchain created.*/
    void initSwapChain();

    void createSwapChain();
    void recreateSwapChain();
    void cleanupSwapChain();

    void createImageViews();

    void createDepthBuffer();
    void createDevice();
    void createUniformBuffers();
    void createRenderPass();
    void createFramebuffers();

    void recordCommandBuffer(
        VkCommandBuffer commandBuffer,
        uint32_t imageIndex,
        TickData* tickData
    );
    void renderImGui();
    void updateUniformBufferData(uint32_t frameIndex);

    void createSynchronizationObjects();

    /**
     * @brief Initializes Vulkan.
     *
     */
    void initVulkan();

    virtual void drawFrame(TickData* tickData);

    virtual void postCleanup() {};

    // validation layers, enable under debug mode only
#ifdef NDEBUG
    const bool enableValidationLayers = false;
#else
    const bool enableValidationLayers = true;
#endif
    static inline const std::vector<const char*> VALIDATION_LAYERS
        = {"VK_LAYER_KHRONOS_validation"};
    static inline const std::vector<const char*> DEVICE_EXTENSIONS
        = {VK_KHR_SWAPCHAIN_EXTENSION_NAME,
#if __APPLE__
           VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME
#endif // __APPLE__
    };
    bool checkValidationLayerSupport();

    // debug messenger setup
    static VkResult CreateDebugUtilsMessengerEXT(
        VkInstance instance,
        const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
        const VkAllocationCallbacks* pAllocator,
        VkDebugUtilsMessengerEXT* pDebugMessenger
    );
    void populateDebugMessengerCreateInfo(
        VkDebugUtilsMessengerCreateInfoEXT& createInfo
    );
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

    static void framebufferResizeCallback(
        GLFWwindow* window,
        int width,
        int height
    );

    const int SOME_VAR = 0;

    GLFWwindow* _window;
    VkInstance _instance;
    VkSurfaceKHR _surface;
    VkDebugUtilsMessengerEXT _debugMessenger;

    // devices

    // swapchain
    VkSwapchainKHR _swapChain = VK_NULL_HANDLE;
    VkFormat _swapChainImageFormat;
    VkExtent2D _swapChainExtent; // resolution of the swapchain images
    struct
    {
        std::vector<VkFramebuffer> frameBuffer;
        std::vector<VkImage> image;
        std::vector<VkImageView> imageView;
    } _swapChainData; // sets of data, ith element of each vec corrsponds to ith image

    // main render pass, and currently the only render pass
    VkRenderPass _mainRenderPass = VK_NULL_HANDLE;

    std::vector<VkSemaphore> _semaImageAvailable;
    std::vector<VkSemaphore> _semaRenderFinished;
    std::vector<VkFence> _fenceInFlight;

    // depth buffer
    VkImage _depthImage;
    VkDeviceMemory _depthImageMemory;
    VkImageView _depthImageView;

    bool _framebufferResized = false;

    uint32_t _currentFrame = 0;

    ImGuiManager _imguiManager;

    std::shared_ptr<VQDevice> _device;

    glm::mat4 _viewMatrix;
    bool viewMode = false;

    std::function<void()> _imguiRenderCallback;

    DeletionStack _deletionStack;

    PhongRenderSystem* _phongSystem;
    EntityViewerSystem* _entityViewerSystem;

    DeltaTimer _deltaTimer;
    Camera _mainCamera = Camera(0, 0, 0, 0, 0, 0);
    InputManager _inputManager;

    bool _lockCursor;
    void cursorPosCallback(GLFWwindow* window, double xpos, double ypos);

    void bindDefaultInputs();
};
