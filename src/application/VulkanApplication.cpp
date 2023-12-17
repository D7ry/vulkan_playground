#include "VulkanApplication.h"
#include "utils/ShaderUtils.h"
#include <GLFW/glfw3.h>
#include <vulkan/vulkan_core.h>

void VulkanApplication::Run() {
        INFO("Initializing Vulkan Application...");
        this->initWindow();
        this->initVulkan();
        this->mainLoop();
        this->cleanup();
}

void VulkanApplication::initWindow() {
        INFO("Initializing GLFW...");
        glfwInit();
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
        this->_window = glfwCreateWindow(800, 600, "Vulkan", nullptr, nullptr);
        if (this->_window == nullptr) {
                ERROR("Failed to create GLFW window.");
        }
        INFO("GLFW initialized; window created.");
}

void VulkanApplication::mainLoop() {
        INFO("Entering main render loop...");
        while (!glfwWindowShouldClose(this->_window)) {
                glfwPollEvents();
        }
        INFO("Main loop exited.");
}

void VulkanApplication::initVulkan() {
        INFO("Initializing Vulkan...");
        this->createInstance();
        if (this->enableValidationLayers) {
                this->setupDebugMessenger();
        }
        this->createSurface();
        this->pickPhysicalDevice();
        this->createLogicalDevice();
        this->createSwapChain();
        this->createImageViews();
        this->createRenderPass();
        this->createGraphicsPipeline();
        INFO("Vulkan initialized.");
}

bool VulkanApplication::checkValidationLayerSupport() {
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

void VulkanApplication::createInstance() {
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
                createInfo.enabledLayerCount = static_cast<uint32_t>(VALIDATION_LAYERS.size());
                createInfo.ppEnabledLayerNames = VALIDATION_LAYERS.data();

                this->populateDebugMessengerCreateInfo(debugMessengerCreateInfo);
                createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debugMessengerCreateInfo;
        } else {
                createInfo.enabledLayerCount = 0;
                createInfo.pNext = nullptr;
        }

        VkResult result = vkCreateInstance(&createInfo, nullptr, &this->_instance);

        if (result != VK_SUCCESS) {
                FATAL("Failed to create Vulkan instance.");
        }

        INFO("Vulkan instance created.");
}

void VulkanApplication::createSurface() {
        VkResult result = glfwCreateWindowSurface(this->_instance, this->_window, nullptr, &this->_surface);
        if (result != VK_SUCCESS) {
                FATAL("Failed to create window surface.");
        }
}

VkResult VulkanApplication::CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
                                                         const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger) {
        INFO("setting up debug messenger .... \n");
        auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");

        if (func != nullptr) {
                return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
        } else {
                ERROR("Failed to set up debug messenger. Function \"vkCreateDebugUtilsMessengerEXT\" not found.");
                return VK_ERROR_EXTENSION_NOT_PRESENT;
        }
}
void VulkanApplication::populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo) {
        createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                                     VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                                 VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        createInfo.pfnUserCallback = debugCallback;
}
void VulkanApplication::setupDebugMessenger() {
        if (!this->enableValidationLayers) {
                ERROR("Validation layers are not enabled, cannot set up debug messenger.");
        }

        VkDebugUtilsMessengerCreateInfoEXT createInfo;
        populateDebugMessengerCreateInfo(createInfo);

        if (CreateDebugUtilsMessengerEXT(this->_instance, &createInfo, nullptr, &this->_debugMessenger) != VK_SUCCESS) {
                FATAL("Failed to set up debug messenger!");
        }
}
VKAPI_ATTR VkBool32 VKAPI_CALL VulkanApplication::debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                                                                VkDebugUtilsMessageTypeFlagsEXT messageType,
                                                                const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData) {
        std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;

        return VK_FALSE;
}
VulkanApplication::QueueFamilyIndices VulkanApplication::findQueueFamilies(VkPhysicalDevice device) {
        INFO("Finding graphics and presentation queue families...");

        QueueFamilyIndices indices;
        uint32_t queueFamilyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);
        std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount); // initialize vector to store queue familieis
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());
        int i = 0;
        for (const auto& queueFamily : queueFamilies) {
                VkBool32 presentationSupport = false;
                if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
                        indices.graphicsFamily = i;
                        INFO("Graphics family found at {}", i);
                }
                vkGetPhysicalDeviceSurfaceSupportKHR(device, i, this->_surface, &presentationSupport);
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
bool VulkanApplication::checkDeviceExtensionSupport(VkPhysicalDevice device) {
        uint32_t extensionCount;
        vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

        std::vector<VkExtensionProperties> availableExtensions(extensionCount);
        vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

        std::set<std::string> requiredExtensions(this->DEVICE_EXTENSIONS.begin(), this->DEVICE_EXTENSIONS.end());

        for (const auto& extension : availableExtensions) {
                requiredExtensions.erase(extension.extensionName);
        }

        return requiredExtensions.empty();
}
bool VulkanApplication::isDeviceSuitable(VkPhysicalDevice device) {
        VkPhysicalDeviceProperties deviceProperties;
        VkPhysicalDeviceFeatures deviceFeatures;
        vkGetPhysicalDeviceProperties(device, &deviceProperties);
        vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

        if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU && deviceFeatures.geometryShader) {
                QueueFamilyIndices indices = this->findQueueFamilies(device);       // look for queue familieis
                return indices.isComplete() && checkDeviceExtensionSupport(device); // found graphics queue
        } else {
                return false;
        }
}
void VulkanApplication::pickPhysicalDevice() {
        INFO("Picking physical device \n");
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
        this->_physicalDevice = physicalDevice; // set current instance's physical device.
}
void VulkanApplication::createLogicalDevice() {
        INFO("Creating logical device...");
        QueueFamilyIndices indices = this->findQueueFamilies(this->_physicalDevice);
        std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
        std::set<uint32_t> uniqueQueueFamilies = {indices.graphicsFamily.value(), indices.presentationFamily.value()};

        VkPhysicalDeviceFeatures deviceFeatures{}; // no features for no
        VkDeviceCreateInfo createInfo{};
        float queuePriority = 1.f;
        for (uint32_t queueFamily : uniqueQueueFamilies) {
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
        createInfo.enabledExtensionCount = static_cast<uint32_t>(this->DEVICE_EXTENSIONS.size());
        createInfo.ppEnabledExtensionNames = DEVICE_EXTENSIONS.data(); // enable swapchain extension
        if (vkCreateDevice(this->_physicalDevice, &createInfo, nullptr, &this->_logicalDevice) != VK_SUCCESS) {
                FATAL("Failed to create logical device!");
        }
        vkGetDeviceQueue(this->_logicalDevice, indices.graphicsFamily.value(), 0,
                         &this->_graphicsQueue); // store graphics queueCreateInfoCount
        vkGetDeviceQueue(this->_logicalDevice, indices.presentationFamily.value(), 0, &this->_presentationQueue);
        INFO("Logical device created.");
}
void VulkanApplication::createSwapChain() {
        INFO("creating swapchain...\n");
        SwapChainSupportDetails swapChainSupport = querySwapChainSupport(_physicalDevice);
        VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
        VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
        VkExtent2D extent = chooseSwapExtent(swapChainSupport.capabilities);

        uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
        if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount) {
                imageCount = swapChainSupport.capabilities.maxImageCount;
        }

        INFO("Populating swapchain create info...\n");
        VkSwapchainCreateInfoKHR createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        createInfo.surface = _surface;

        createInfo.minImageCount = imageCount;
        createInfo.imageFormat = surfaceFormat.format;
        createInfo.imageColorSpace = surfaceFormat.colorSpace;
        createInfo.imageExtent = extent;
        createInfo.imageArrayLayers = 1;
        createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

        QueueFamilyIndices indices = findQueueFamilies(_physicalDevice);
        uint32_t queueFamilyIndices[] = {indices.graphicsFamily.value(), indices.presentationFamily.value()};

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

        if (vkCreateSwapchainKHR(this->_logicalDevice, &createInfo, nullptr, &_swapChain) != VK_SUCCESS) {
                FATAL("Failed to create swap chain!");
        }

        vkGetSwapchainImagesKHR(this->_logicalDevice, _swapChain, &imageCount, nullptr);
        _swapChainImages.resize(imageCount);
        vkGetSwapchainImagesKHR(this->_logicalDevice, _swapChain, &imageCount, _swapChainImages.data());

        _swapChainImageFormat = surfaceFormat.format;
        _swapChainExtent = extent;
        INFO("Swap chain created!\n");
}
VulkanApplication::SwapChainSupportDetails VulkanApplication::querySwapChainSupport(VkPhysicalDevice device) {
        SwapChainSupportDetails details;

        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, _surface, &details.capabilities);

        uint32_t formatCount;
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, _surface, &formatCount, nullptr);

        if (formatCount != 0) {
                details.formats.resize(formatCount);
                vkGetPhysicalDeviceSurfaceFormatsKHR(device, _surface, &formatCount, details.formats.data());
        }

        uint32_t presentModeCount;
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, _surface, &presentModeCount, nullptr);

        if (presentModeCount != 0) {
                details.presentModes.resize(presentModeCount);
                vkGetPhysicalDeviceSurfacePresentModesKHR(device, _surface, &presentModeCount, details.presentModes.data());
        }

        return details;
}
VkSurfaceFormatKHR VulkanApplication::chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) {
        for (const auto& availableFormat : availableFormats) {
                if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
                        return availableFormat;
                }
        }

        return availableFormats[0];
}
VkPresentModeKHR VulkanApplication::chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes) {
        for (const auto& availablePresentMode : availablePresentModes) {
                if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
                        return availablePresentMode;
                }
        }

        return VK_PRESENT_MODE_FIFO_KHR;
}
VkExtent2D VulkanApplication::chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities) {
        if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
                return capabilities.currentExtent;
        } else {
                int width, height;
                glfwGetFramebufferSize(this->_window, &width, &height);

                VkExtent2D actualExtent = {static_cast<uint32_t>(width), static_cast<uint32_t>(height)};

                actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
                actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

                return actualExtent;
        }
}
void VulkanApplication::createImageViews() {
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
                if (vkCreateImageView(this->_logicalDevice, &createInfo, nullptr, &_swapChainImageViews[i]) != VK_SUCCESS) {
                        FATAL("Failed to create image views!");
                }
        }
        INFO("Image views created.");
}
void VulkanApplication::createGraphicsPipeline() {
        ERROR("Base vulkan application does not have a graphics pipeline.");
}

//https://vulkan-tutorial.com/Drawing_a_triangle/Graphics_pipeline_basics/Render_passes
void VulkanApplication::createRenderPass() {
        ERROR("Base vulkan application does not have a render pass.");
}
void VulkanApplication::cleanup() {
        INFO("Cleaning up...");
        vkDestroyPipeline(this->_logicalDevice, this->_graphicsPipeline, nullptr);
        vkDestroyPipelineLayout(this->_logicalDevice, this->_pipelineLayout, nullptr);
        this->destroyImageViews();
        vkDestroyPipelineLayout(this->_logicalDevice, this->_pipelineLayout, nullptr);
        vkDestroyRenderPass(this->_logicalDevice, this->_renderPass, nullptr);
        vkDestroySwapchainKHR(this->_logicalDevice, this->_swapChain, nullptr);
        vkDestroyDevice(this->_logicalDevice, nullptr);
        vkDestroySurfaceKHR(this->_instance, this->_surface, nullptr);
        vkDestroyInstance(this->_instance, nullptr);
        glfwDestroyWindow(this->_window);
        glfwTerminate();
        INFO("Resource cleaned up.");
}
void VulkanApplication::destroyImageViews() {
        for (auto imageView : this->_swapChainImageViews) {
                vkDestroyImageView(this->_logicalDevice, imageView, nullptr);
        }
}
