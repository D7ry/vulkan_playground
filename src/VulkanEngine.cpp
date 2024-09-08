#include "components/TextureManager.h"
#include <algorithm>
#include <cinttypes>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <functional>
#include <iostream>
#include <limits>
#include <set>

#include <cstddef>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vulkan/vk_enum_string_helper.h>
#include <vulkan/vulkan_core.h>

#include "VulkanEngine.h"
#include "components/Camera.h"

static Entity* entityInstanced = new Entity("instanced entity");
static Entity* entityInstanced2 = new Entity("instanced entity2");

void VulkanEngine::initGLFW() {
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
    if (!glfwVulkanSupported()) {
        FATAL("Vulkan is not supported on this machine!");
    }
    this->_window = glfwCreateWindow(
        DEFAULTS::WINDOW_WIDTH,
        DEFAULTS::WINDOW_HEIGHT,
        "Vulkan Engine",
        nullptr,
        nullptr
    );
    if (this->_window == nullptr) {
        FATAL("Failed to initialize GLFW windlw!");
    }
    glfwSetWindowUserPointer(_window, this);
    _deletionStack.push([this]() {
        glfwDestroyWindow(this->_window);
        glfwTerminate();
    });
}

void VulkanEngine::cursorPosCallback(
    GLFWwindow* window,
    double xpos,
    double ypos
) {
    static bool updatedCursor = false;
    static int prevX = -1;
    static int prevY = -1;
    if (!updatedCursor) {
        prevX = xpos;
        prevY = ypos;
        updatedCursor = true;
    }
    double deltaX = prevX - xpos;
    double deltaY = prevY - ypos;
    // handle camera movement
    deltaX *= 0.3;
    deltaY *= 0.3; // make movement slower
    if (_lockCursor) {
        _mainCamera.ModRotation(deltaX, deltaY, 0);
    }
    prevX = xpos;
    prevY = ypos;
}

void VulkanEngine::Init() {
    initGLFW();
    { // Input Handling
        auto keyCallback
            = [](GLFWwindow* window, int key, int scancode, int action, int mods
              ) {
                  VulkanEngine* pThis = reinterpret_cast<VulkanEngine*>(
                      glfwGetWindowUserPointer(window)
                  );
                  pThis->_inputManager.OnKeyInput(
                      window, key, scancode, action, mods
                  );
              };
        glfwSetKeyCallback(this->_window, keyCallback);
        // don't have a mouse input manager yet, so manually bind cursor pos
        // callback
        auto cursorPosCallback
            = [](GLFWwindow* window, double xpos, double ypos) {
                  VulkanEngine* pThis = reinterpret_cast<VulkanEngine*>(
                      glfwGetWindowUserPointer(window)
                  );
                  pThis->cursorPosCallback(window, xpos, ypos);
              };
        glfwSetCursorPosCallback(this->_window, cursorPosCallback);
        bindDefaultInputs();
    }

    INFO("Initializing Render Manager...");
    glfwSetFramebufferSizeCallback(_window, this->framebufferResizeCallback);
    this->initVulkan();
    TextureManager::GetSingleton()->Init(_device
    ); // pass device to texture manager for it to start loading
    this->_deletionStack.push([this]() {
        TextureManager::GetSingleton()->Cleanup();
    });

    // create static engine ubo
    {
        for (VQBuffer& engineUBO : _engineUBOStatic)
            _device->CreateBufferInPlace(
                sizeof(EngineUBOStatic),
                VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
                    | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                engineUBO
            );
        _deletionStack.push([this]() {
            for (VQBuffer& engineUBO : _engineUBOStatic)
                engineUBO.Cleanup();
        });
    }
    { // lab to mess around with ecs
        this->_entityViewerSystem = new EntityViewerSystem();
        this->_phongSystem = new PhongRenderSystem();
        this->_phongSystemInstanced = new PhongRenderSystemInstanced();
        this->_globalGridSystem = new GlobalGridSystem();
        InitData initData;
        { // populate initData
            initData.device = this->_device.get();
            initData.textureManager = TextureManager::GetSingleton(
            ); // TODO: get rid of singleton pattern
            initData.swapChainImageFormat = this->_swapChainImageFormat;
            initData.renderPass.mainPass = _mainRenderPass;
            for (int i = 0; i < _engineUBOStatic.size(); i++) {
                initData.engineUBOStaticDescriptorBufferInfo[i].range
                    = sizeof(EngineUBOStatic);
                initData.engineUBOStaticDescriptorBufferInfo[i].buffer
                    = _engineUBOStatic[i].buffer;
                initData.engineUBOStaticDescriptorBufferInfo[i].offset = 0;
            }
        }

        _phongSystemInstanced->Init(&initData);
        _deletionStack.push([this]() { this->_phongSystemInstanced->Cleanup(); }
        );

        _phongSystem->Init(&initData);
        _deletionStack.push([this]() { this->_phongSystem->Cleanup(); });

        _globalGridSystem->Init(&initData);
        _deletionStack.push([this]() { this->_globalGridSystem->Cleanup(); });

        // make instanced entity
        {
            auto phongMeshComponent
                = _phongSystemInstanced
                      ->MakePhongRenderSystemInstancedComponent(
                          "../resources/viking_room.obj",
                          "../resources/viking_room.png",
                          10
                      );
            TransformComponent* transformComponent = new TransformComponent();
            *transformComponent = TransformComponent::Identity();
            entityInstanced->AddComponent(transformComponent);
            entityInstanced->AddComponent(phongMeshComponent);
            _phongSystemInstanced->AddEntity(entityInstanced);
            _entityViewerSystem->AddEntity(entityInstanced);
            phongMeshComponent->FlagAsDirty(entityInstanced);

            // auto phongMeshComponent2
            //     = _phongSystemInstanced
            //           ->MakePhongRenderSystemInstancedComponent(
            //               "../resources/spot.obj", "../resources/spot.png",
            //               10
            //           );
            // entityInstanced2->AddComponent(new TransformComponent());
            // entityInstanced2->AddComponent(phongMeshComponent2);
            // phongMeshComponent2->FlagAsDirty(entityInstanced2);

            // let's go crazy
            for (int i = 0; i < 10000; i++) {
                Entity* spot = new Entity("Spot");
                spot->AddComponent(new TransformComponent());
                spot->AddComponent(
                    _phongSystemInstanced
                        ->MakePhongRenderSystemInstancedComponent(
                            "../resources/spot.obj",
                            "../resources/spot.png",
                            10000 // give it a large hint so don't need to
                                  // resize
                        )
                );
                // Generate spherical coordinates
                float radius = 30.0f;
                float theta = static_cast<float>(rand()) / RAND_MAX * 2 * M_PI;
                float phi = acos(2 * static_cast<float>(rand()) / RAND_MAX - 1);

                // Convert spherical coordinates to Cartesian
                float x = radius * sin(phi) * cos(theta);
                float y = radius * sin(phi) * sin(theta);
                float z = radius * cos(phi);

                // Set the position
                spot->GetComponent<TransformComponent>()->position.x = x;
                spot->GetComponent<TransformComponent>()->position.y = y;
                spot->GetComponent<TransformComponent>()->position.z = z;
                spot->GetComponent<PhongRenderSystemInstancedComponent>()
                    ->FlagAsDirty(spot);
                _phongSystemInstanced->AddEntity(spot);
            }

            _phongSystemInstanced->AddEntity(entityInstanced2);
            _entityViewerSystem->AddEntity(entityInstanced2);
        }
    }
}

void VulkanEngine::Run() {
    while (!glfwWindowShouldClose(_window)) {
        Tick();
    }
}

void VulkanEngine::Tick() {
    {
        PROFILE_SCOPE(&_profiler, "Main Tick");
        glfwPollEvents();
        _deltaTimer.Tick();
        double deltaTime = _deltaTimer.GetDeltaTime();
        _inputManager.Tick(deltaTime);
        TickData tickData{&_mainCamera, deltaTime};

        tickData.profiler = &_profiler;
        drawImGui();

        flushEngineUBOStatic(_currentFrame);
        drawFrame(&tickData, _currentFrame);
        _currentFrame = (_currentFrame + 1) % NUM_FRAME_IN_FLIGHT;
        vkDeviceWaitIdle(this->_device->logicalDevice);
    }
    _profiler.NewProfile();
}

void VulkanEngine::framebufferResizeCallback(
    GLFWwindow* window,
    int width,
    int height
) {
    DEBUG("Window resized to {}x{}", width, height);
    auto app
        = reinterpret_cast<VulkanEngine*>(glfwGetWindowUserPointer(window));
    app->_framebufferResized = true;
}

void VulkanEngine::createDevice() {
    VkPhysicalDevice physicalDevice = this->pickPhysicalDevice();
    this->_device = std::make_shared<VQDevice>(physicalDevice);
    this->_device->InitQueueFamilyIndices(this->_surface);
    this->_device->CreateLogicalDeviceAndQueue(DEVICE_EXTENSIONS);
    this->_device->CreateGraphicsCommandPool();
    this->_device->CreateGraphicsCommandBuffer(NUM_FRAME_IN_FLIGHT);
    this->_deletionStack.push([this]() { this->_device->Cleanup(); });
}

void VulkanEngine::initVulkan() {
    INFO("Initializing Vulkan...");
    this->createInstance();
    if (this->enableValidationLayers) {
        // this->setupDebugMessenger();
    }
    this->createSurface();
    this->createDevice();
    this->initSwapChain();
    this->createImageViews();
    this->createRenderPass();
    this->createDepthBuffer();
    this->createSynchronizationObjects();
    this->_imguiManager.InitializeRenderPass(
        this->_device->logicalDevice, _swapChainImageFormat
    );
    this->createFramebuffers();
    { // init misc imgui resources
        this->_imguiManager.InitializeImgui();
        this->_imguiManager.InitializeDescriptorPool(
            NUM_FRAME_IN_FLIGHT, _device->logicalDevice
        );
        this->_imguiManager.BindVulkanResources(
            _window,
            _instance,
            _device->physicalDevice,
            _device->logicalDevice,
            _device->queueFamilyIndices.graphicsFamily.value(),
            _device->graphicsQueue,
            _swapChainData.frameBuffer.size()
        );
    }
    this->_deletionStack.push([this]() {
        this->_imguiManager.Cleanup(_device->logicalDevice);
    });
    INFO("Vulkan initialized.");
}

bool VulkanEngine::checkValidationLayerSupport() {
    uint32_t layerCount;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

    std::vector<VkLayerProperties> availableLayers(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

    for (const char* layerName : VALIDATION_LAYERS) {
        bool layerFound = false;
        for (const auto& layerProperties : availableLayers) {
            if (strcmp(layerName, layerProperties.layerName) == 0) {
                layerFound = true;
                break;
            }
        }
        if (!layerFound) {
            return false;
        }
    }
    return true;
}

void VulkanEngine::createInstance() {
    INFO("Creating Vulkan instance...");
    if (this->enableValidationLayers) {
        if (!this->checkValidationLayerSupport()) {
            ERROR("Validation layers requested, but not available!");
        } else {
            INFO("Validation layers requested, and available.");
        }
    }
    // initialize and populate application info
    INFO("Populating Vulkan application info...");
    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = this->APPLICATION_NAME;
    appInfo.applicationVersion = this->APPLICATION_VERSION;
    appInfo.pEngineName = this->ENGINE_NAME;
    appInfo.engineVersion = this->ENGINE_VERSION;
    appInfo.apiVersion = this->API_VERSION;

    INFO("Populating Vulkan instance create info...");
    // initialize and populate createInfo, which contains the application info
    VkInstanceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;

    std::vector<const char*> instanceExtensions;
    // get glfw Extensions
    {
        uint32_t glfwExtensionCount = 0;
        const char** glfwExtensions
            = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
        for (int i = 0; i < glfwExtensionCount; i++) {
            instanceExtensions.push_back(glfwExtensions[i]);
        }
    }
// https://stackoverflow.com/questions/72789012/why-does-vkcreateinstance-return-vk-error-incompatible-driver-on-macos-despite
#if __APPLE__
    // enable extensions for apple vulkan translation
    instanceExtensions.push_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
    createInfo.flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
#endif // __APPLE__
#ifndef NDEBUG
    instanceExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
#endif // NDEBUG
    createInfo.enabledExtensionCount = instanceExtensions.size();
    createInfo.ppEnabledExtensionNames = instanceExtensions.data();

    VkDebugUtilsMessengerCreateInfoEXT debugMessengerCreateInfo{};

    createInfo.enabledLayerCount = 0;
    if (this->enableValidationLayers) { // populate debug messenger create info
        createInfo.enabledLayerCount
            = static_cast<uint32_t>(VALIDATION_LAYERS.size());
        createInfo.ppEnabledLayerNames = VALIDATION_LAYERS.data();

        this->populateDebugMessengerCreateInfo(debugMessengerCreateInfo);
        createInfo.pNext
            = (VkDebugUtilsMessengerCreateInfoEXT*)&debugMessengerCreateInfo;
    } else {
        createInfo.enabledLayerCount = 0;
        createInfo.pNext = nullptr;
    }
    VkResult result = vkCreateInstance(&createInfo, nullptr, &this->_instance);

    if (result != VK_SUCCESS) {
        FATAL(
            "Failed to create Vulkan instance; Result: {}",
            string_VkResult(result)
        );
    }
    _deletionStack.push([this]() {
        vkDestroyInstance(this->_instance, nullptr);
    });

    INFO("Vulkan instance created.");
}

void VulkanEngine::createSurface() {
    VkResult result = glfwCreateWindowSurface(
        this->_instance, this->_window, nullptr, &this->_surface
    );
    if (result != VK_SUCCESS) {
        FATAL("Failed to create window surface.");
    }
    _deletionStack.push([this]() {
        vkDestroySurfaceKHR(this->_instance, this->_surface, nullptr);
    });
}

VkResult VulkanEngine::CreateDebugUtilsMessengerEXT(
    VkInstance instance,
    const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
    const VkAllocationCallbacks* pAllocator,
    VkDebugUtilsMessengerEXT* pDebugMessenger
) {
    INFO("setting up debug messenger .... ");
    auto func = (PFN_vkCreateDebugUtilsMessengerEXT
    )vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");

    if (func != nullptr) {
        return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
    } else {
        ERROR("Failed to set up debug messenger. Function "
              "\"vkCreateDebugUtilsMessengerEXT\" not found.");
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
}

void VulkanEngine::populateDebugMessengerCreateInfo(
    VkDebugUtilsMessengerCreateInfoEXT& createInfo
) {
    createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    createInfo.messageSeverity
        = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT
          | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT
          | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT
                             | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT
                             | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    createInfo.pfnUserCallback = debugCallback;
}

void VulkanEngine::setupDebugMessenger() {
    if (!this->enableValidationLayers) {
        ERROR(
            "Validation layers are not enabled, cannot set up debug messenger."
        );
    }

    VkDebugUtilsMessengerCreateInfoEXT createInfo;
    populateDebugMessengerCreateInfo(createInfo);

    if (CreateDebugUtilsMessengerEXT(
            this->_instance, &createInfo, nullptr, &this->_debugMessenger
        )
        != VK_SUCCESS) {
        FATAL("Failed to set up debug messenger!");
    }
}

VKAPI_ATTR VkBool32 VKAPI_CALL VulkanEngine::debugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* pUserData
) {
    std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;
    return VK_FALSE;
}

VulkanEngine::QueueFamilyIndices VulkanEngine::findQueueFamilies(
    VkPhysicalDevice device
) {
    INFO("Finding graphics and presentation queue families...");

    QueueFamilyIndices indices;
    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(
        device, &queueFamilyCount, nullptr
    );
    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount
    ); // initialize vector to store queue familieis
    vkGetPhysicalDeviceQueueFamilyProperties(
        device, &queueFamilyCount, queueFamilies.data()
    );
    int i = 0;
    for (const auto& queueFamily : queueFamilies) {
        VkBool32 presentationSupport = false;
        if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            indices.graphicsFamily = i;
            INFO("Graphics family found at {}", i);
        }
        vkGetPhysicalDeviceSurfaceSupportKHR(
            device, i, this->_surface, &presentationSupport
        );
        if (presentationSupport) {
            indices.presentationFamily = i;
            INFO("Presentation family found at {}", i);
        }
        if (indices.isComplete()) {
            break;
        }
        i++;
    }
    std::optional<uint32_t> graphicsFamily;
    return indices;
}

bool VulkanEngine::checkDeviceExtensionSupport(VkPhysicalDevice device) {
    DEBUG("checking device extension support");
    uint32_t extensionCount;
    vkEnumerateDeviceExtensionProperties(
        device, nullptr, &extensionCount, nullptr
    );

    std::vector<VkExtensionProperties> availableExtensions(extensionCount);
    vkEnumerateDeviceExtensionProperties(
        device, nullptr, &extensionCount, availableExtensions.data()
    );

    std::set<std::string> requiredExtensions(
        this->DEVICE_EXTENSIONS.begin(), this->DEVICE_EXTENSIONS.end()
    );

    for (const auto& extension : availableExtensions) {
        requiredExtensions.erase(extension.extensionName);
    }

    return requiredExtensions.empty();
}

bool VulkanEngine::isDeviceSuitable(VkPhysicalDevice device) {
    DEBUG("checking is device suitable");
    VkPhysicalDeviceProperties deviceProperties;
    VkPhysicalDeviceFeatures deviceFeatures;
    vkGetPhysicalDeviceProperties(device, &deviceProperties);
    vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

    if (
#if !__APPLE__
        deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU &&
        deviceFeatures.geometryShader &&
#endif
        true) {
        QueueFamilyIndices indices
            = this->findQueueFamilies(device); // look for queue familieis
        return indices.isComplete()
               && checkDeviceExtensionSupport(device); // found graphics queue
    } else {
        return false;
    }
}

// pick a physical device that satisfies `isDeviceSuitable()`
VkPhysicalDevice VulkanEngine::pickPhysicalDevice() {
    INFO("Picking physical device ");
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(this->_instance, &deviceCount, nullptr);
    if (deviceCount == 0) {
        FATAL("Failed to find any GPU!");
    }
    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(_instance, &deviceCount, devices.data());
    for (const auto& device : devices) {
        if (isDeviceSuitable(device)) {
            physicalDevice = device;
            break;
        }
    }
    if (physicalDevice == VK_NULL_HANDLE) {
        FATAL("Failed to find a suitable GPU!");
    }

    INFO("Physical device picked.");
    return physicalDevice;
}

void VulkanEngine::createSwapChain() {
    INFO("creating swapchain...");
    SwapChainSupportDetails swapChainSupport
        = querySwapChainSupport(_device->physicalDevice);
    VkSurfaceFormatKHR surfaceFormat
        = chooseSwapSurfaceFormat(swapChainSupport.formats);
    VkPresentModeKHR presentMode
        = chooseSwapPresentMode(swapChainSupport.presentModes);
    VkExtent2D extent = chooseSwapExtent(swapChainSupport.capabilities);

    uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
    if (swapChainSupport.capabilities.maxImageCount > 0
        && imageCount > swapChainSupport.capabilities.maxImageCount) {
        imageCount = swapChainSupport.capabilities.maxImageCount;
    }

    INFO("Populating swapchain create info...");
    VkSwapchainCreateInfoKHR createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = _surface;

    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = surfaceFormat.format;
    createInfo.imageColorSpace = surfaceFormat.colorSpace;
    createInfo.imageExtent = extent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    QueueFamilyIndices indices = findQueueFamilies(_device->physicalDevice);
    uint32_t queueFamilyIndices[]
        = {indices.graphicsFamily.value(), indices.presentationFamily.value()};

    if (indices.graphicsFamily != indices.presentationFamily) {
        createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices = queueFamilyIndices;
    } else {
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    }

    createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.presentMode = presentMode;
    createInfo.clipped = VK_TRUE;

    createInfo.oldSwapchain = VK_NULL_HANDLE;

    if (vkCreateSwapchainKHR(
            this->_device->logicalDevice, &createInfo, nullptr, &_swapChain
        )
        != VK_SUCCESS) {
        FATAL("Failed to create swap chain!");
    }

    vkGetSwapchainImagesKHR(
        this->_device->logicalDevice, _swapChain, &imageCount, nullptr
    );
    _swapChainData.image.resize(imageCount);
    _swapChainData.imageView.resize(imageCount);
    _swapChainData.frameBuffer.resize(imageCount);

    vkGetSwapchainImagesKHR(
        this->_device->logicalDevice,
        _swapChain,
        &imageCount,
        _swapChainData.image.data()
    );

    _swapChainImageFormat = surfaceFormat.format;
    _swapChainExtent = extent;
    INFO("Swap chain created!\n");
}

void VulkanEngine::cleanupSwapChain() {
    INFO("Cleaning up swap chain...");
    vkDestroyImageView(_device->logicalDevice, _depthImageView, nullptr);
    vkDestroyImage(_device->logicalDevice, _depthImage, nullptr);
    vkFreeMemory(_device->logicalDevice, _depthImageMemory, nullptr);

    _imguiManager.DestroyFrameBuffers(_device->logicalDevice);
    for (VkFramebuffer framebuffer : this->_swapChainData.frameBuffer) {
        vkDestroyFramebuffer(
            this->_device->logicalDevice, framebuffer, nullptr
        );
    }
    for (VkImageView imageView : this->_swapChainData.imageView) {
        vkDestroyImageView(this->_device->logicalDevice, imageView, nullptr);
    }
    vkDestroySwapchainKHR(
        this->_device->logicalDevice, this->_swapChain, nullptr
    );
}

void VulkanEngine::recreateSwapChain() {
    // need to recreate render pass for HDR changing, we're not doing that for
    // now
    INFO("Recreating swap chain...");
    // handle window minimization
    int width = 0, height = 0;
    glfwGetFramebufferSize(_window, &width, &height);
    while (width == 0 || height == 0
    ) { // when the window is minimized, wait for it to be restored
        glfwGetFramebufferSize(_window, &width, &height);
        glfwWaitEvents();
    }
    // wait for device to be idle
    vkDeviceWaitIdle(_device->logicalDevice);
    this->cleanupSwapChain();

    this->createSwapChain();
    this->createImageViews();
    this->createDepthBuffer();
    this->createFramebuffers();
    INFO("Swap chain recreated.");
}

VulkanEngine::SwapChainSupportDetails VulkanEngine::querySwapChainSupport(
    VkPhysicalDevice device
) {
    SwapChainSupportDetails details;

    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(
        device, _surface, &details.capabilities
    );

    uint32_t formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(
        device, _surface, &formatCount, nullptr
    );

    if (formatCount != 0) {
        details.formats.resize(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(
            device, _surface, &formatCount, details.formats.data()
        );
    }

    uint32_t presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(
        device, _surface, &presentModeCount, nullptr
    );

    if (presentModeCount != 0) {
        details.presentModes.resize(presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(
            device, _surface, &presentModeCount, details.presentModes.data()
        );
    }

    return details;
}

VkSurfaceFormatKHR VulkanEngine::chooseSwapSurfaceFormat(
    const std::vector<VkSurfaceFormatKHR>& availableFormats
) {
    for (const auto& availableFormat : availableFormats) {
        if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB
            && availableFormat.colorSpace
                   == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            return availableFormat;
        }
    }

    return availableFormats[0];
}

VkPresentModeKHR VulkanEngine::chooseSwapPresentMode(
    const std::vector<VkPresentModeKHR>& availablePresentModes
) {
    for (const auto& availablePresentMode : availablePresentModes) {
        if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
            return availablePresentMode;
        }
    }

    return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D VulkanEngine::chooseSwapExtent(
    const VkSurfaceCapabilitiesKHR& capabilities
) {
    if (capabilities.currentExtent.width
        != std::numeric_limits<uint32_t>::max()) {
        return capabilities.currentExtent;
    } else {
        int width, height;
        glfwGetFramebufferSize(this->_window, &width, &height);

        VkExtent2D actualExtent
            = {static_cast<uint32_t>(width), static_cast<uint32_t>(height)};

        actualExtent.width = std::clamp(
            actualExtent.width,
            capabilities.minImageExtent.width,
            capabilities.maxImageExtent.width
        );
        actualExtent.height = std::clamp(
            actualExtent.height,
            capabilities.minImageExtent.height,
            capabilities.maxImageExtent.height
        );

        return actualExtent;
    }
}

void VulkanEngine::createImageViews() {
    for (size_t i = 0; i < this->_swapChainData.image.size(); i++) {
        VkImageViewCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        createInfo.image = _swapChainData.image[i];
        createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        createInfo.format = _swapChainImageFormat;
        createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        createInfo.subresourceRange.baseMipLevel = 0;
        createInfo.subresourceRange.levelCount = 1;
        createInfo.subresourceRange.baseArrayLayer = 0;
        createInfo.subresourceRange.layerCount = 1;
        if (vkCreateImageView(
                this->_device->logicalDevice,
                &createInfo,
                nullptr,
                &_swapChainData.imageView[i]
            )
            != VK_SUCCESS) {
            FATAL("Failed to create image views!");
        }
    }
    INFO("Image views created.");
}

void VulkanEngine::createSynchronizationObjects() {
    INFO("Creating synchronization objects...");
    this->_semaImageAvailable.resize(NUM_FRAME_IN_FLIGHT);
    this->_semaRenderFinished.resize(NUM_FRAME_IN_FLIGHT);
    this->_fenceInFlight.resize(NUM_FRAME_IN_FLIGHT);

    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags
        = VK_FENCE_CREATE_SIGNALED_BIT; // create with a signaled bit so that
                                        // the 1st frame can start right away
    for (size_t i = 0; i < NUM_FRAME_IN_FLIGHT; i++) {
        if (vkCreateSemaphore(
                _device->logicalDevice,
                &semaphoreInfo,
                nullptr,
                &_semaImageAvailable[i]
            ) != VK_SUCCESS
            || vkCreateSemaphore(
                   _device->logicalDevice,
                   &semaphoreInfo,
                   nullptr,
                   &_semaRenderFinished[i]
               ) != VK_SUCCESS
            || vkCreateFence(
                   _device->logicalDevice,
                   &fenceInfo,
                   nullptr,
                   &_fenceInFlight[i]
               ) != VK_SUCCESS) {
            FATAL("Failed to create synchronization objects for a frame!");
        }
    }
    this->_deletionStack.push([this]() {
        for (size_t i = 0; i < NUM_FRAME_IN_FLIGHT; i++) {
            vkDestroySemaphore(
                this->_device->logicalDevice,
                this->_semaRenderFinished[i],
                nullptr
            );
            vkDestroySemaphore(
                this->_device->logicalDevice,
                this->_semaImageAvailable[i],
                nullptr
            );
            vkDestroyFence(
                this->_device->logicalDevice, this->_fenceInFlight[i], nullptr
            );
        }
    });
}

void VulkanEngine::Cleanup() {
    INFO("Cleaning up...");
    _deletionStack.flush();
    if (enableValidationLayers) {
        if (this->_debugMessenger != nullptr) {
            // TODO: implement this
            // vkDestroyDebugUtilsMessengerEXT(_instance, _debugMessenger,
            // nullptr);
        }
    }
    postCleanup();
    INFO("Resource cleaned up.");
}

void VulkanEngine::createRenderPass() {
    INFO("Creating render pass...");
    VkAttachmentDescription colorAttachment{};
    colorAttachment.format = this->_swapChainImageFormat;
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;

    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;

    // don't care about stencil
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    // colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    colorAttachment.finalLayout
        = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL; // for imgui

    VkAttachmentReference colorAttachmentRef{};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentDescription depthAttachment{};
    depthAttachment.format
        = VulkanUtils::findDepthFormat(_device->physicalDevice);
    depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    depthAttachment.finalLayout
        = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentReference depthAttachmentRef{};
    depthAttachmentRef.attachment = 1;
    depthAttachmentRef.layout
        = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    // set up subpass

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;
    subpass.pDepthStencilAttachment = &depthAttachmentRef;

    // dependency to make sure that the render pass waits for the image to be
    // available before drawing
    VkSubpassDependency dependency{};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
                              | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
                              | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT
                               | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

    std::array<VkAttachmentDescription, 2> attachments
        = {colorAttachment, depthAttachment};

    VkRenderPassCreateInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
    renderPassInfo.pAttachments = attachments.data();
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies = &dependency;

    if (vkCreateRenderPass(
            _device->logicalDevice,
            &renderPassInfo,
            nullptr,
            &this->_mainRenderPass
        )
        != VK_SUCCESS) {
        FATAL("Failed to create render pass!");
    }

    _deletionStack.push([this]() {
        vkDestroyRenderPass(
            this->_device->logicalDevice, this->_mainRenderPass, nullptr
        );
    });
}

void VulkanEngine::createFramebuffers() {
    // iterate through image views and create framebuffers
    if (_mainRenderPass == VK_NULL_HANDLE) {
        FATAL("Render pass is null!");
    }
    for (size_t i = 0; i < _swapChainData.image.size(); i++) {
        VkImageView attachments[]
            = {_swapChainData.imageView[i], _depthImageView};
        VkFramebufferCreateInfo framebufferInfo{};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass
            = _mainRenderPass; // each framebuffer is associated with a render
                               // pass; they need to be compatible i.e. having
                               // same number of attachments and same formats
        framebufferInfo.attachmentCount
            = sizeof(attachments) / sizeof(VkImageView);
        framebufferInfo.pAttachments = attachments;
        framebufferInfo.width = _swapChainExtent.width;
        framebufferInfo.height = _swapChainExtent.height;
        framebufferInfo.layers = 1; // number of layers in image arrays
        if (vkCreateFramebuffer(
                _device->logicalDevice,
                &framebufferInfo,
                nullptr,
                &_swapChainData.frameBuffer[i]
            )
            != VK_SUCCESS) {
            FATAL("Failed to create framebuffer!");
        }
    }
    _imguiManager.InitializeFrameBuffer(
        this->_swapChainData.image.size(),
        _device->logicalDevice,
        _swapChainData.imageView,
        _swapChainExtent
    );
    INFO("Framebuffers created.");
}

void VulkanEngine::recordCommandBuffer(
    VkCommandBuffer commandBuffer,
    uint32_t imageIndex,
    TickData* tickData
) { // called every frame

    // start render pass
    {
        VkRenderPassBeginInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassInfo.renderPass = this->_mainRenderPass;
        renderPassInfo.framebuffer
            = this->_swapChainData.frameBuffer[imageIndex];

        renderPassInfo.renderArea.offset = {0, 0};
        renderPassInfo.renderArea.extent = this->_swapChainExtent;

        std::array<VkClearValue, 2> clearValues{};
        clearValues[0].color = {{0.0f, 0.0f, 0.0f, 1.0f}};
        clearValues[1].depthStencil = {1.0f, 0};

        renderPassInfo.clearValueCount
            = static_cast<uint32_t>(clearValues.size());
        renderPassInfo.pClearValues = clearValues.data();

        vkCmdBeginRenderPass(
            commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE
        );

        // TODO: this doesn't need to update every frame
        VkViewport viewport{};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = static_cast<float>(_swapChainExtent.width);
        viewport.height = static_cast<float>(_swapChainExtent.height);
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
        vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

        VkRect2D scissor{};
        scissor.offset = {0, 0};
        scissor.extent = _swapChainExtent;
        vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

        vkCmdEndRenderPass(commandBuffer);
    }
}

void VulkanEngine::createDepthBuffer() {
    INFO("Creating depth buffer...");
    VkFormat depthFormat = VulkanUtils::findBestFormat(
        {VK_FORMAT_D32_SFLOAT,
         VK_FORMAT_D32_SFLOAT_S8_UINT,
         VK_FORMAT_D24_UNORM_S8_UINT},
        VK_IMAGE_TILING_OPTIMAL,
        VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT,
        _device->physicalDevice
    );
    VulkanUtils::createImage(
        _swapChainExtent.width,
        _swapChainExtent.height,
        depthFormat,
        VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        _depthImage,
        _depthImageMemory,
        _device->physicalDevice,
        _device->logicalDevice
    );
    _depthImageView = VulkanUtils::createImageView(
        _depthImage,
        _device->logicalDevice,
        depthFormat,
        VK_IMAGE_ASPECT_DEPTH_BIT
    );
}

void VulkanEngine::flushEngineUBOStatic(uint8_t frame) {
    VQBuffer& buf = _engineUBOStatic[frame];
    EngineUBOStatic ubo{
        _mainCamera.GetViewMatrix() // view
        // proj
    };
    getMainProjectionMatrix(ubo.proj);
    memcpy(buf.bufferAddress, &ubo, sizeof(ubo));
}

void VulkanEngine::drawFrame(TickData* tickData, uint8_t frame) {
    //  Wait for the previous frame to finish
    vkWaitForFences(
        _device->logicalDevice,
        1,
        &this->_fenceInFlight[frame],
        VK_TRUE,
        UINT64_MAX
    );

    //  Acquire an image from the swap chain
    uint32_t imageIndex;
    VkResult result = vkAcquireNextImageKHR(
        this->_device->logicalDevice,
        _swapChain,
        UINT64_MAX,
        _semaImageAvailable[frame],
        VK_NULL_HANDLE,
        &imageIndex
    );
    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
        this->recreateSwapChain();
        return;
    } else if (result != VK_SUCCESS) {
        FATAL("Failed to acquire swap chain image!");
    }

    // lock the fence
    vkResetFences(
        this->_device->logicalDevice, 1, &this->_fenceInFlight[frame]
    );

    VkFramebuffer FB = this->_swapChainData.frameBuffer[imageIndex];
    VkCommandBuffer CB = this->_device->graphicsCommandBuffers[frame];
    //  Record a command buffer which draws the scene onto that image
    vkResetCommandBuffer(CB, 0);

    { // update tickData->graphics field
        tickData->graphics.currentFrameInFlight = frame;
        tickData->graphics.currentSwapchainImageIndex = imageIndex;
        tickData->graphics.currentCB
            = this->_device->graphicsCommandBuffers[frame];
        tickData->graphics.currentFB = FB;
        tickData->graphics.currentFBextend = this->_swapChainExtent;
        tickData->graphics.mainProjectionMatrix = glm::perspective(
            glm::radians(DEFAULTS::FOV),
            _swapChainExtent.width / (float)_swapChainExtent.height,
            0.1f,
            100.f
        );
        tickData->graphics.mainProjectionMatrix[1][1]
            *= -1; // invert y axis because vulkan
    }

    { // begin command buffer
        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = 0;                  // Optional
        beginInfo.pInheritanceInfo = nullptr; // Optional

        if (vkBeginCommandBuffer(CB, &beginInfo) != VK_SUCCESS) {
            FATAL("Failed to begin recording command buffer!");
        }
    }

    {     // invoke all GraphicsSystem under the main render pass
        { // begin main render pass
            VkRenderPassBeginInfo renderPassInfo{};
            renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
            renderPassInfo.renderPass = this->_mainRenderPass;
            renderPassInfo.framebuffer = FB;

            renderPassInfo.renderArea.offset = {0, 0};
            renderPassInfo.renderArea.extent = _swapChainExtent;

            std::array<VkClearValue, 2> clearValues{};
            clearValues[0].color = {{0.0f, 0.0f, 0.0f, 1.0f}};
            clearValues[1].depthStencil = {1.0f, 0};

            renderPassInfo.clearValueCount
                = static_cast<uint32_t>(clearValues.size());
            renderPassInfo.pClearValues = clearValues.data();

            vkCmdBeginRenderPass(
                CB, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE
            );
        }
        { // set viewport and scissor
            VkViewport viewport{};
            viewport.x = 0.0f;
            viewport.y = 0.0f;
            viewport.width = static_cast<float>(_swapChainExtent.width);
            viewport.height = static_cast<float>(_swapChainExtent.height);
            viewport.minDepth = 0.0f;
            viewport.maxDepth = 1.0f;
            vkCmdSetViewport(CB, 0, 1, &viewport);

            VkRect2D scissor{};
            scissor.offset = {0, 0};
            scissor.extent = _swapChainExtent;
            vkCmdSetScissor(CB, 0, 1, &scissor);
        }
        this->_phongSystem->Tick(tickData);
        this->_phongSystemInstanced->Tick(tickData);
        this->_globalGridSystem->Tick(tickData);
        { // end main render pass
            vkCmdEndRenderPass(CB);
        }
    }

    _imguiManager.RecordCommandBuffer(tickData);
    { // end command buffer
        if (vkEndCommandBuffer(CB) != VK_SUCCESS) {
            FATAL("Failed to record command buffer!");
        }
    }

    //  Submit the recorded command buffer
    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    VkSemaphore waitSemaphores[]
        = {_semaImageAvailable[frame]}; // use imageAvailable semaphore
                                        // to make sure that the image
                                        // is available before drawing
    VkPipelineStageFlags waitStages[]
        = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    std::array<VkCommandBuffer, 1> submitCommandBuffers
        = {this->_device->graphicsCommandBuffers[frame]};

    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;
    submitInfo.commandBufferCount
        = static_cast<uint32_t>(submitCommandBuffers.size());
    submitInfo.pCommandBuffers = submitCommandBuffers.data();

    VkSemaphore signalSemaphores[] = {_semaRenderFinished[frame]};
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    // the submission does not start until vkAcquireNextImageKHR returns, and
    // downs the corresponding _semaRenderFinished semapohre once it's done.
    if (vkQueueSubmit(
            _device->graphicsQueue, 1, &submitInfo, _fenceInFlight[frame]
        )
        != VK_SUCCESS) {
        FATAL("Failed to submit draw command buffer!");
    }

    //  Present the swap chain image
    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

    // set up semaphore, so that  after submitting to the queue, we wait  for
    // the
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores
        = signalSemaphores; // wait for render to finish before presenting

    VkSwapchainKHR swapChains[] = {_swapChain};
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapChains;
    presentInfo.pImageIndices = &imageIndex; // specify which image to present
    presentInfo.pResults = nullptr; // Optional: can be used to check if
                                    // presentation was successful

    // the present doesn't happen until the render is finished, and the
    // semaphore is signaled(result of vkQueueSubimt)
    result = vkQueuePresentKHR(_device->presentationQueue, &presentInfo);
    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR
        || this->_framebufferResized) {
        this->recreateSwapChain();
        this->_framebufferResized = false;
    } else if (result != VK_SUCCESS) {
        FATAL("Failed to present swap chain image!");
    }
}

void VulkanEngine::initSwapChain() {
    createSwapChain();
    this->_deletionStack.push([this]() { this->cleanupSwapChain(); });
}

void VulkanEngine::drawImGui() {
    PROFILE_SCOPE(&_profiler, "ImGui Draw");
    _imguiManager.BeginImGuiContext();
    if (ImGui::Begin("Vulkan Engine")) {
        ImGui::SetWindowPos(ImVec2(0, 0), ImGuiCond_Once);
        ImGui::SetWindowSize(ImVec2(400, 400), ImGuiCond_Once);
        ImGui::Text("Framerate: %f", 1 / _deltaTimer.GetDeltaTime());
        ImGui::Separator();
        ImGui::SeparatorText("Camera");
        {
            ImGui::Text(
                "Position: (%f, %f, %f)",
                _mainCamera.GetPosition().x,
                _mainCamera.GetPosition().y,
                _mainCamera.GetPosition().z
            );
            ImGui::Text(
                "Yaw: %f Pitch: %f Roll: %f",
                _mainCamera.GetRotation().y,
                _mainCamera.GetRotation().x,
                _mainCamera.GetRotation().z
            );
            if (_lockCursor) {
                ImGui::Text("View Mode: Active");
            } else {
                ImGui::Text("View Mode: Deactive");
            }
        }

        ImGui::SeparatorText("Profiler");
        {
            std::unique_ptr<std::vector<Profiler::Entry>> lastProfileData
                = _profiler.GetLastProfile();
            for (Profiler::Entry& entry : *lastProfileData) {
                ImGui::Text("level: %i", entry.level);
                ImGui::Text("%s ms", entry.name);
                ImGui::Text(
                    "Time: %f",
                    std::chrono::
                        duration<double, std::chrono::milliseconds::period>(
                            entry.end - entry.begin
                        )
                            .count()
                );
            }
        }
    }
    ImGui::End(); // VulkanEngine
    _entityViewerSystem->DrawImGui();
    _imguiManager.EndImGuiContext();
}

void VulkanEngine::bindDefaultInputs() {
    const int CAMERA_SPEED = 3;
    _inputManager.RegisterCallback(
        GLFW_KEY_W,
        InputManager::KeyCallbackCondition::HOLD,
        [this]() {
            _mainCamera.Move(_deltaTimer.GetDeltaTime() * CAMERA_SPEED, 0, 0);
        }
    );
    _inputManager.RegisterCallback(
        GLFW_KEY_S,
        InputManager::KeyCallbackCondition::HOLD,
        [this]() {
            _mainCamera.Move(-_deltaTimer.GetDeltaTime() * CAMERA_SPEED, 0, 0);
        }
    );
    _inputManager.RegisterCallback(
        GLFW_KEY_A,
        InputManager::KeyCallbackCondition::HOLD,
        [this]() {
            _mainCamera.Move(0, _deltaTimer.GetDeltaTime() * CAMERA_SPEED, 0);
        }
    );
    _inputManager.RegisterCallback(
        GLFW_KEY_D,
        InputManager::KeyCallbackCondition::HOLD,
        [this]() {
            _mainCamera.Move(0, -_deltaTimer.GetDeltaTime() * CAMERA_SPEED, 0);
        }
    );
    _inputManager.RegisterCallback(
        GLFW_KEY_LEFT_CONTROL,
        InputManager::KeyCallbackCondition::HOLD,
        [this]() {
            _mainCamera.Move(0, 0, -CAMERA_SPEED * _deltaTimer.GetDeltaTime());
        }
    );
    _inputManager.RegisterCallback(
        GLFW_KEY_SPACE,
        InputManager::KeyCallbackCondition::HOLD,
        [this]() {
            _mainCamera.Move(0, 0, CAMERA_SPEED * _deltaTimer.GetDeltaTime());
        }
    );

    _inputManager.RegisterCallback(
        GLFW_KEY_TAB,
        InputManager::KeyCallbackCondition::PRESS,
        [this]() {
            _lockCursor = !_lockCursor;
            if (_lockCursor) {
                glfwSetInputMode(_window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
                ImGui::GetIO().ConfigFlags
                    |= (ImGuiConfigFlags_NoMouse | ImGuiConfigFlags_NoKeyboard);
            } else {
                glfwSetInputMode(_window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
                ImGui::GetIO().ConfigFlags &= ~ImGuiConfigFlags_NoMouse;
                ImGui::GetIO().ConfigFlags &= ~ImGuiConfigFlags_NoKeyboard;
            }
        }
    );
    _inputManager.RegisterCallback(
        GLFW_KEY_ESCAPE,
        InputManager::KeyCallbackCondition::PRESS,
        [this]() { glfwSetWindowShouldClose(_window, GLFW_TRUE); }
    );
}

void VulkanEngine::getMainProjectionMatrix(glm::mat4& projectionMatrix) {
    projectionMatrix = glm::perspective(
        glm::radians(_FOV),
        _swapChainExtent.width / (float)_swapChainExtent.height,
        DEFAULTS::ZNEAR,
        DEFAULTS::ZFAR
    );
    projectionMatrix[1][1] *= -1; // invert for vulkan coord system
}
