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
#include "MeshInstance.h"
#include "MeshRenderManager.h"
#include <GLFW/glfw3.h>
#include <vulkan/vulkan_core.h>

#include "VulkanEngine.h"

void VulkanEngine::Init(GLFWwindow* window) {
    INFO("Initializing Render Manager...");
    this->_window = window;
    glfwSetFramebufferSizeCallback(_window, this->framebufferResizeCallback);
    this->initVulkan();
    TextureManager::GetSingleton()->Init(_device
    ); // pass device to texture manager for it to start loading
    this->_meshRenderManager = std::make_unique<MeshRenderManager>();
    this->_deletionStack.push([this]() {
        TextureManager::GetSingleton()->Cleanup();
    });
    this->_deletionStack.push([this]() { this->_meshRenderManager->Cleanup(); }
    );
}

#define CAMERA_SPEED 3

void VulkanEngine::SetViewMatrix(glm::mat4 viewMatrix) {
    _viewMatrix = viewMatrix;
}

void VulkanEngine::Tick(float deltaTime) {
    _imguiManager.RenderFrame();
    drawFrame();
    vkDeviceWaitIdle(this->_device->logicalDevice);
}

void VulkanEngine::framebufferResizeCallback(
    GLFWwindow* window,
    int width,
    int height
) {
    INFO("Window resized to {}x{}", width, height);
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
    this->_device->CreateGraphicsCommandBuffer(NUM_INTERMEDIATE_FRAMES);
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

    this->_imguiManager.InitializeImgui();
    this->_imguiManager.InitializeDescriptorPool(
        NUM_INTERMEDIATE_FRAMES, _device->logicalDevice
    );
    this->_imguiManager.BindVulkanResources(
        _window,
        _instance,
        _device->physicalDevice,
        _device->logicalDevice,
        _device->queueFamilyIndices.graphicsFamily.value(),
        _device->graphicsQueue,
        _swapChainFrameBuffers.size()
    );
    this->_imguiManager.InitializeCommandPoolAndBuffers(
        NUM_INTERMEDIATE_FRAMES,
        _device->logicalDevice,
        _device->queueFamilyIndices.graphicsFamily.value()
    );

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
    // get glfw Extensions
    uint32_t glfwExtensionCount = 0;

    INFO("Getting GLFW extensions...");
    const char** glfwExtensions;
    glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
    createInfo.enabledExtensionCount = glfwExtensionCount;
    createInfo.ppEnabledExtensionNames = glfwExtensions;

    INFO("GLFW extensions:");
    for (uint32_t i = 0; i < glfwExtensionCount; i++) {
        INFO("{}", glfwExtensions[i]);
    }

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
        FATAL("Failed to create Vulkan instance.");
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
    VkPhysicalDeviceProperties deviceProperties;
    VkPhysicalDeviceFeatures deviceFeatures;
    vkGetPhysicalDeviceProperties(device, &deviceProperties);
    vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

    if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU
        && deviceFeatures.geometryShader) {
        QueueFamilyIndices indices
            = this->findQueueFamilies(device); // look for queue familieis
        return indices.isComplete()
               && checkDeviceExtensionSupport(device); // found graphics queue
    } else {
        return false;
    }
}

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
    _swapChainImages.resize(imageCount);
    vkGetSwapchainImagesKHR(
        this->_device->logicalDevice,
        _swapChain,
        &imageCount,
        _swapChainImages.data()
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
    for (VkFramebuffer framebuffer : this->_swapChainFrameBuffers) {
        vkDestroyFramebuffer(
            this->_device->logicalDevice, framebuffer, nullptr
        );
    }
    for (VkImageView imageView : this->_swapChainImageViews) {
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
    INFO("Creating {} image views...", this->_swapChainImages.size());
    this->_swapChainImageViews.resize(this->_swapChainImages.size());
    for (size_t i = 0; i < this->_swapChainImages.size(); i++) {
        VkImageViewCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        createInfo.image = _swapChainImages[i];
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
                &_swapChainImageViews[i]
            )
            != VK_SUCCESS) {
            FATAL("Failed to create image views!");
        }
    }
    INFO("Image views created.");
}

void VulkanEngine::createSynchronizationObjects() {
    INFO("Creating synchronization objects...");
    this->_semaImageAvailable.resize(NUM_INTERMEDIATE_FRAMES);
    this->_semaRenderFinished.resize(NUM_INTERMEDIATE_FRAMES);
    this->_fenceInFlight.resize(NUM_INTERMEDIATE_FRAMES);

    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags
        = VK_FENCE_CREATE_SIGNALED_BIT; // create with a signaled bit so that
                                        // the 1st frame can start right away
    for (size_t i = 0; i < NUM_INTERMEDIATE_FRAMES; i++) {
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
        for (size_t i = 0; i < NUM_INTERMEDIATE_FRAMES; i++) {
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
            _device->logicalDevice, &renderPassInfo, nullptr, &this->_renderPass
        )
        != VK_SUCCESS) {
        FATAL("Failed to create render pass!");
    }

    _deletionStack.push([this]() {
        vkDestroyRenderPass(
            this->_device->logicalDevice, this->_renderPass, nullptr
        );
    });
}

void VulkanEngine::createFramebuffers() {
    INFO("Creating {} framebuffers...", this->_swapChainImageViews.size());
    this->_swapChainFrameBuffers.resize(this->_swapChainImageViews.size());
    // iterate through image views and create framebuffers
    if (_renderPass == VK_NULL_HANDLE) {
        FATAL("Render pass is null!");
    }
    for (size_t i = 0; i < _swapChainImageViews.size(); i++) {
        VkImageView attachments[] = {_swapChainImageViews[i], _depthImageView};
        VkFramebufferCreateInfo framebufferInfo{};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass
            = _renderPass; // each framebuffer is associated with a render pass;
                           // they need to be compatible i.e. having same number
                           // of attachments and same formats
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
                &_swapChainFrameBuffers[i]
            )
            != VK_SUCCESS) {
            FATAL("Failed to create framebuffer!");
        }
    }
    _imguiManager.InitializeFrameBuffer(
        this->_swapChainImageViews.size(),
        _device->logicalDevice,
        _swapChainImageViews,
        _swapChainExtent
    );
    INFO("Framebuffers created.");
}

void VulkanEngine::recordCommandBuffer(
    VkCommandBuffer commandBuffer,
    uint32_t imageIndex
) { // called every frame
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = 0;                  // Optional
    beginInfo.pInheritanceInfo = nullptr; // Optional

    if (vkBeginCommandBuffer(
            this->_device->graphicsCommandBuffers[_currentFrame], &beginInfo
        )
        != VK_SUCCESS) {
        FATAL("Failed to begin recording command buffer!");
    }

    // start render pass
    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = this->_renderPass;
    renderPassInfo.framebuffer = this->_swapChainFrameBuffers[imageIndex];

    renderPassInfo.renderArea.offset = {0, 0};
    renderPassInfo.renderArea.extent = this->_swapChainExtent;

    std::array<VkClearValue, 2> clearValues{};
    clearValues[0].color = {{0.0f, 0.0f, 0.0f, 1.0f}};
    clearValues[1].depthStencil = {1.0f, 0};

    renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
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

    _meshRenderManager->RecordRenderCommands(commandBuffer, _currentFrame);
    vkCmdEndRenderPass(commandBuffer);

    if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
        FATAL("Failed to record command buffer!");
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

void VulkanEngine::drawFrame() {
    //  Wait for the previous frame to finish
    vkWaitForFences(
        _device->logicalDevice,
        1,
        &this->_fenceInFlight[this->_currentFrame],
        VK_TRUE,
        UINT64_MAX
    );

    //  Acquire an image from the swap chain
    uint32_t imageIndex;
    VkResult result = vkAcquireNextImageKHR(
        this->_device->logicalDevice,
        _swapChain,
        UINT64_MAX,
        _semaImageAvailable[this->_currentFrame],
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
        this->_device->logicalDevice,
        1,
        &this->_fenceInFlight[this->_currentFrame]
    );

    //  Record a command buffer which draws the scene onto that image
    vkResetCommandBuffer(
        this->_device->graphicsCommandBuffers[this->_currentFrame], 0
    );
    this->recordCommandBuffer(
        this->_device->graphicsCommandBuffers[this->_currentFrame], imageIndex
    );
    _imguiManager.RecordCommandBuffer(
        this->_currentFrame, imageIndex, _swapChainExtent
    );

    _meshRenderManager->UpdateUniformBuffers(
        _currentFrame,
        _viewMatrix,
        glm::perspective(
            glm::radians(90.f),
            _swapChainExtent.width / (float)_swapChainExtent.height,
            0.1f,
            100.f
        )
    );

    //  Submit the recorded command buffer
    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    VkSemaphore waitSemaphores[]
        = {_semaImageAvailable[_currentFrame]}; // use imageAvailable semaphore
                                                // to make sure that the image
                                                // is available before drawing
    VkPipelineStageFlags waitStages[]
        = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    std::array<VkCommandBuffer, 2> submitCommandBuffers
        = {this->_device->graphicsCommandBuffers[_currentFrame],
           _imguiManager.GetCommandBuffer(_currentFrame)};

    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;
    submitInfo.commandBufferCount
        = static_cast<uint32_t>(submitCommandBuffers.size());
    submitInfo.pCommandBuffers = submitCommandBuffers.data();

    VkSemaphore signalSemaphores[] = {_semaRenderFinished[_currentFrame]};
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    // the submission does not start until vkAcquireNextImageKHR returns, and
    // downs the corresponding _semaRenderFinished semapohre once it's done.
    if (vkQueueSubmit(
            _device->graphicsQueue,
            1,
            &submitInfo,
            _fenceInFlight[_currentFrame]
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
    //  Advance to the next frame
    _currentFrame = (_currentFrame + 1) % NUM_INTERMEDIATE_FRAMES;
}

void VulkanEngine::SetImguiRenderCallback(std::function<void()> imguiFunction) {
    this->_imguiManager.BindRenderCallback(imguiFunction);
}

void VulkanEngine::Prepare() {
    this->_meshRenderManager->PrepareRendering(
        NUM_INTERMEDIATE_FRAMES, _renderPass, _device
    );
}

void VulkanEngine::initSwapChain() {
    createSwapChain();
    this->_deletionStack.push([this]() { this->cleanupSwapChain(); });
}
