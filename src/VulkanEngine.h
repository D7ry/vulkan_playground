#pragma once
// stl
#include <cstdint>
#include <memory>
#include <optional>
#include <filesystem>

// vulkan
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>
#if __APPLE__
#include <vulkan/vulkan_beta.h> // VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME, for molten-vk support
#endif

// vq library
#include "lib/VQBuffer.h"
#include "lib/VQDevice.h"

// structs
#include "structs/TickData.h"

// ecs systems
#include "ecs/system/EntityViewerSystem.h"
#include "ecs/system/GlobalGridSystem.h"
#include "ecs/system/PhongRenderSystem.h"
#include "ecs/system/PhongRenderSystemInstanced.h"

// Engine Components
#include "components/Camera.h"
#include "components/DeletionStack.h"
#include "components/DeltaTimer.h"
#include "components/ImGuiManager.h"
#include "components/InputManager.h"
#include "components/Profiler.h"
#include "components/TextureManager.h"

class TickData;

class VulkanEngine
{
  public:
    // Engine-wide static UBO that gets updated every Tick()
    // systems can read from `InitData::engineUBOStaticDescriptorBufferInfo`
    // to bind to the UBO in their own graphics pipelines.
    struct EngineUBOStatic
    {
        glm::mat4 view;              // view matrix
        glm::mat4 proj;              // proj matrix
        float timeSinceStartSeconds; // time in seconds since engine start
        float sinWave;               // a number interpolating between [0,1]
    };

    void Init();
    void Run();
    void Tick();
    void Cleanup();

  private:
    struct QueueFamilyIndices
    {
        std::optional<uint32_t> graphicsFamily;
        std::optional<uint32_t> presentationFamily;
    };

    struct SwapChainSupportDetails
    {
        VkSurfaceCapabilitiesKHR capabilities;
        std::vector<VkSurfaceFormatKHR> formats;
        std::vector<VkPresentModeKHR> presentModes;
    };

    /* ---------- Initialization Subroutines ---------- */
    void initGLFW();
    void initVulkan();
    void createInstance();
    void createSurface();
    void createDevice();
    void createRenderPass(); // create main render pass
    void createFramebuffers();
    void createSynchronizationObjects();

    /* ---------- Physical Device Selection ---------- */
    QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device);
    bool checkDeviceExtensionSupport(VkPhysicalDevice device);
    bool isDeviceSuitable(VkPhysicalDevice device);
    VkPhysicalDevice pickPhysicalDevice();

    /* ---------- Swapchain ---------- */
    SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device);
    VkSurfaceFormatKHR chooseSwapSurfaceFormat(
        const std::vector<VkSurfaceFormatKHR>& availableFormats
    );
    VkPresentModeKHR chooseSwapPresentMode(
        const std::vector<VkPresentModeKHR>& availablePresentModes
    );
    VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);
    void initSwapChain();
    void createSwapChain();
    void recreateSwapChain();
    void cleanupSwapChain();
    void createImageViews();
    void createDepthBuffer();

    // required device extensions
    static inline const std::vector<const char*> DEVICE_EXTENSIONS = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,
#if __APPLE__ // molten vk support
        VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME,
    // VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME
#endif // __APPLE__
    };
    //  NOTE: appple M3 does not have
    //  `VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME` which moltenVK
    //  requires to enable for metal compatbility. disabling it leads to a
    //  trivial validation layer error that may be safely ignored.

    bool checkValidationLayerSupport();

    /* ---------- Debug Utilities ---------- */
    void populateDebugMessengerCreateInfo(
        VkDebugUtilsMessengerCreateInfoEXT& createInfo
    );
    void setupDebugMessenger(); // unused for now
    static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
        VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
        VkDebugUtilsMessageTypeFlagsEXT messageType,
        const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
        void* pUserData
    );

    /* ---------- Input ---------- */
    static void framebufferResizeCallback(
        GLFWwindow* window,
        int width,
        int height
    );
    void cursorPosCallback(GLFWwindow* window, double xpos, double ypos);
    void bindDefaultInputs();

    /* ---------- Render-Time Functions ---------- */
    void getMainProjectionMatrix(glm::mat4& projectionMatrix);
    void flushEngineUBOStatic(uint8_t frame);
    void drawImGuiPerfPlots();
    void drawImGui();
    void drawFrame(TickData* tickData, uint8_t frame);

    // record command buffer to perform some example GPU
    // operations. currently not used anymore
    void recordCommandBuffer(
        VkCommandBuffer commandBuffer,
        uint32_t imageIndex,
        TickData* tickData
    );

    /* ---------- Top-level data ---------- */
    GLFWwindow* _window;
    VkInstance _instance;
    VkSurfaceKHR _surface;
    VkDebugUtilsMessengerEXT _debugMessenger;

    /* ---------- swapchain ---------- */
    VkSwapchainKHR _swapChain = VK_NULL_HANDLE;
    VkFormat _swapChainImageFormat;
    VkExtent2D _swapChainExtent; // resolution of the swapchain images

    struct
    {
        std::vector<VkFramebuffer> frameBuffer;
        std::vector<VkImage> image;
        std::vector<VkImageView> imageView;
    } _swapChainData; // each element corresponds to one image in the swap chain

    /* ---------- Synchronization Primivites ---------- */
    struct EngineSynchronizationPrimitives
    {
        VkSemaphore semaImageAvailable;
        VkSemaphore semaRenderFinished;
        VkFence fenceInFlight;
    };
    std::array<EngineSynchronizationPrimitives, NUM_FRAME_IN_FLIGHT> _synchronizationPrimitives;

    /* ---------- Depth Buffer ---------- */
    VkImage _depthImage;
    VkDeviceMemory _depthImageMemory;
    VkImageView _depthImageView;

    /* ---------- Render Passes ---------- */
    // main render pass, and currently the only render pass
    VkRenderPass _mainRenderPass = VK_NULL_HANDLE;

    /* ---------- Tick-dynamic Data ---------- */
    bool _framebufferResized = false;
    uint8_t _currentFrame = 0;
    // whether we are locking the cursor within the glfw window
    bool _lockCursor;

    std::shared_ptr<VQDevice> _device;
    std::function<void()> _imguiRenderCallback;

    // static ubo for each frame
    // each buffer stores a `EngineUBOStatic`
    std::array<VQBuffer, NUM_FRAME_IN_FLIGHT> _engineUBOStatic;

    float _FOV = 90;
    float _timeSinceStartSeconds; // seconds in time since engine start

    /* ---------- Systems from ecs ---------- */
    // TODO: SystemManager
    PhongRenderSystem* _phongSystem;
    PhongRenderSystemInstanced* _phongSystemInstanced;
    GlobalGridSystem* _globalGridSystem;
    EntityViewerSystem* _entityViewerSystem;

    /* ---------- Engine Components ---------- */
    DeletionStack _deletionStack;
    TextureManager _textureManager;
    ImGuiManager _imguiManager;
    DeltaTimer _deltaTimer;
    Camera _mainCamera;
    InputManager _inputManager;
    Profiler _profiler;
};
