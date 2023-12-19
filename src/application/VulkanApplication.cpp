#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <limits>
#include <set>

#include "VulkanApplication.h"
#include <GLFW/glfw3.h>
#include <cstddef>
#include <vulkan/vulkan_core.h>

void VulkanApplication::Run() {
        INFO("Initializing Vulkan Application...");
        this->initWindow();
        this->initVulkan();
        this->mainLoop();
        this->cleanup();
}

void VulkanApplication::keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
        INFO("{} {} {} {}", key, scancode, action, mods);
        if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
                glfwSetWindowShouldClose(window, GLFW_TRUE);
        }
}

void VulkanApplication::setKeyCallback() {
        INFO("Setting up key callback...");
        auto keyCallback = [](GLFWwindow* window, int key, int scancode, int action, int mods) {
                auto app = reinterpret_cast<VulkanApplication*>(glfwGetWindowUserPointer(window));
                app->keyCallback(window, key, scancode, action, mods);
        };

        glfwSetKeyCallback(this->_window, keyCallback);
}

void VulkanApplication::updateUniformBufferData(uint32_t frameIndex) {
        ERROR("Base vulkan application does not have a uniform buffer.");
}

void VulkanApplication::initWindow() {
        INFO("Initializing GLFW...");
        glfwInit();
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
        if (!glfwVulkanSupported()) {
                FATAL("Vulkan is not supported on this machine!");
        }
        this->_window = glfwCreateWindow(800, 600, "Vulkan", nullptr, nullptr);
        glfwSetWindowUserPointer(_window, this);
        glfwSetFramebufferSizeCallback(_window, this->framebufferResizeCallback);
        // set up glfw input callbacks
        if (this->_window == nullptr) {
                ERROR("Failed to create GLFW window.");
        }
        this->setKeyCallback();
        INFO("GLFW initialized; window created.");
}

void VulkanApplication::mainLoop() {
        INFO("Entering main render loop...");
        while (!glfwWindowShouldClose(this->_window)) {
                glfwPollEvents();
                _imguiManager.RenderFrame();
                drawFrame();
                _deltaTimer.Update();
        }
        vkDeviceWaitIdle(this->_logicalDevice);
        INFO("Main loop exited.");
}

void VulkanApplication::drawFrame() {
        // nothing to draw
}

void VulkanApplication::framebufferResizeCallback(GLFWwindow* window, int width, int height) {
        INFO("Window resized to {}x{}", width, height);
        auto app = reinterpret_cast<VulkanApplication*>(glfwGetWindowUserPointer(window));
        app->_framebufferResized = true;
}

void VulkanApplication::initVulkan() {
        INFO("Initializing Vulkan...");
        this->createInstance();
        if (this->enableValidationLayers) {
                // this->setupDebugMessenger();
        }
        this->createSurface();
        this->pickPhysicalDevice();
        this->createLogicalDevice();
        this->createSwapChain();
        this->createImageViews();
        this->createRenderPass();
        this->createDescriptorSetLayout();
        this->createGraphicsPipeline();
        this->_imguiManager.InitializeImgui();
        this->_imguiManager.InitializeRenderPass(_logicalDevice, _swapChainImageFormat);
        this->createFramebuffers();
        this->createCommandPool();
        this->createVertexBuffer();
        this->createIndexBuffer();
        this->createUniformBuffers();
        this->createDescriptorPool();
        this->createDescriptorSets();
        this->createCommandBuffer();
        this->createSynchronizationObjects();

        INFO("Begin initializing ImGui...");
        auto indices = this->findQueueFamilies(_physicalDevice);
        this->_imguiManager.InitializeDescriptorPool(MAX_FRAMES_IN_FLIGHT, _logicalDevice);
        this->_imguiManager.BindVulkanResources(
                _window, _instance, _physicalDevice, _logicalDevice, indices.graphicsFamily.value(), _graphicsQueue, _swapChainFrameBuffers.size()
        );
        this->_imguiManager.InitializeCommandPoolAndBuffers(MAX_FRAMES_IN_FLIGHT, _logicalDevice, indices.graphicsFamily.value());
        //this->_imguiManager.InitializeFrameBuffer(_swapChainFrameBuffers.size(), _logicalDevice, _swapChainImageViews, _swapChainExtent);
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

VkResult VulkanApplication::CreateDebugUtilsMessengerEXT(
        VkInstance instance,
        const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
        const VkAllocationCallbacks* pAllocator,
        VkDebugUtilsMessengerEXT* pDebugMessenger
) {
        INFO("setting up debug messenger .... ");
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
        createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT
                                     | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT
                                 | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
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
VKAPI_ATTR VkBool32 VKAPI_CALL VulkanApplication::debugCallback(
        VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
        VkDebugUtilsMessageTypeFlagsEXT messageType,
        const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
        void* pUserData
) {
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
        INFO("creating swapchain...");
        SwapChainSupportDetails swapChainSupport = querySwapChainSupport(_physicalDevice);
        VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
        VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
        VkExtent2D extent = chooseSwapExtent(swapChainSupport.capabilities);

        uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
        if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount) {
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
void VulkanApplication::cleanupSwapChain() {
        INFO("Cleaning up swap chain...");
        _imguiManager.DestroyFrameBuffers(_logicalDevice);
        for (VkFramebuffer framebuffer : this->_swapChainFrameBuffers) {
                vkDestroyFramebuffer(this->_logicalDevice, framebuffer, nullptr);
        }
        for (VkImageView imageView : this->_swapChainImageViews) {
                vkDestroyImageView(this->_logicalDevice, imageView, nullptr);
        }
        vkDestroySwapchainKHR(this->_logicalDevice, this->_swapChain, nullptr);
}
void VulkanApplication::recreateSwapChain() {
        // need to recreate render pass for HDR changing, we're not doing that for now
        INFO("Recreating swap chain...");
        // handle window minimization
        int width = 0, height = 0;
        glfwGetFramebufferSize(_window, &width, &height);
        while (width == 0 || height == 0) {
                glfwGetFramebufferSize(_window, &width, &height);
                glfwWaitEvents();
        }
        // wait for device to be idle
        vkDeviceWaitIdle(_logicalDevice);
        this->cleanupSwapChain();

        this->createSwapChain();
        this->createImageViews();
        this->createFramebuffers();
        INFO("Swap chain recreated.");
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
void VulkanApplication::createVertexBuffer() { ERROR("Base vulkan application does not have a vertex buffer."); }

void VulkanApplication::createIndexBuffer() { ERROR("Base vulkan application does not have an index buffer."); }

void VulkanApplication::createUniformBuffers() { ERROR("Base vulkan application does not have a uniform buffer."); }

void VulkanApplication::createDescriptorSetLayout() { ERROR("Base vulkan application does not have a descriptor set layout."); }

void VulkanApplication::createGraphicsPipeline() { ERROR("Base vulkan application does not have a graphics pipeline."); }

// https://vulkan-tutorial.com/Drawing_a_triangle/Graphics_pipeline_basics/Render_passes
void VulkanApplication::createRenderPass() { ERROR("Base vulkan application does not have a render pass."); }

void VulkanApplication::createFramebuffers() { ERROR("Base vulkan application does not have a frame buffer."); }

void VulkanApplication::createCommandPool() { ERROR("Base vulkan application does not have a command pool."); }

void VulkanApplication::createCommandBuffer() { ERROR("Base vulkan application does not have a command buffer."); }

void VulkanApplication::createDescriptorPool() {}

void VulkanApplication::createDescriptorSets() {}

void VulkanApplication::recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex) {
        ERROR("Base vulkan application does not have a command buffer.");
}

void VulkanApplication::createSynchronizationObjects() {
        INFO("Creating synchronization objects...");
        this->_semaImageAvailable.resize(MAX_FRAMES_IN_FLIGHT);
        this->_semaRenderFinished.resize(MAX_FRAMES_IN_FLIGHT);
        this->_fenceInFlight.resize(MAX_FRAMES_IN_FLIGHT);

        VkSemaphoreCreateInfo semaphoreInfo{};
        semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
        VkFenceCreateInfo fenceInfo{};
        fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT; // create with a signaled bit so that the 1st frame can start right away
        for (size_t i = 0; i < this->MAX_FRAMES_IN_FLIGHT; i++) {
                if (vkCreateSemaphore(_logicalDevice, &semaphoreInfo, nullptr, &_semaImageAvailable[i]) != VK_SUCCESS
                    || vkCreateSemaphore(_logicalDevice, &semaphoreInfo, nullptr, &_semaRenderFinished[i]) != VK_SUCCESS
                    || vkCreateFence(_logicalDevice, &fenceInfo, nullptr, &_fenceInFlight[i]) != VK_SUCCESS) {
                        FATAL("Failed to create synchronization objects for a frame!");
                }
        }
}
void VulkanApplication::cleanup() {
        INFO("Cleaning up...");
        _imguiManager.Cleanup(_logicalDevice);
        cleanupSwapChain();
        for (auto& buffer : _uniformBuffers) {
                if (buffer) {
                        vkDestroyBuffer(this->_logicalDevice, buffer, nullptr);
                }
        }
        for (auto& memory : _uniformBuffersMemory) {
                if (memory) {
                        vkFreeMemory(this->_logicalDevice, memory, nullptr);
                }
        }
        if (_vertexBuffer != VK_NULL_HANDLE) {
                vkDestroyBuffer(this->_logicalDevice, _vertexBuffer, nullptr);
        }
        if (_vertexBufferMemory != VK_NULL_HANDLE) {
                vkFreeMemory(this->_logicalDevice, _vertexBufferMemory, nullptr);
        }
        if (_indexBuffer != VK_NULL_HANDLE) {
                vkDestroyBuffer(this->_logicalDevice, _indexBuffer, nullptr);
        }
        if (_indexBufferMemory != VK_NULL_HANDLE) {
                vkFreeMemory(this->_logicalDevice, _indexBufferMemory, nullptr);
        }
        if (_descriptorPool != VK_NULL_HANDLE) {
                vkDestroyDescriptorPool(this->_logicalDevice, _descriptorPool, nullptr);
        }
        if (_descriptorSetLayout != VK_NULL_HANDLE) {
                vkDestroyDescriptorSetLayout(this->_logicalDevice, _descriptorSetLayout, nullptr);
        }

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
                vkDestroySemaphore(this->_logicalDevice, this->_semaRenderFinished[i], nullptr);
                vkDestroySemaphore(this->_logicalDevice, this->_semaImageAvailable[i], nullptr);
                vkDestroyFence(this->_logicalDevice, this->_fenceInFlight[i], nullptr);
        }
        vkDestroyCommandPool(this->_logicalDevice, this->_commandPool, nullptr);

        vkDestroyPipeline(this->_logicalDevice, this->_graphicsPipeline, nullptr);
        vkDestroyPipelineLayout(this->_logicalDevice, this->_pipelineLayout, nullptr);

        vkDestroyRenderPass(this->_logicalDevice, this->_renderPass, nullptr);
        vkDestroyDevice(this->_logicalDevice, nullptr);
        if (enableValidationLayers) {
                if (this->_debugMessenger != nullptr) {
                        // TODO: implement this
                        // vkDestroyDebugUtilsMessengerEXT(_instance, _debugMessenger, nullptr);
                }
        }
        vkDestroySurfaceKHR(this->_instance, this->_surface, nullptr);
        vkDestroyInstance(this->_instance, nullptr);
        glfwDestroyWindow(this->_window);
        glfwTerminate();
        INFO("Resource cleaned up.");
}
uint32_t VulkanApplication::findMemoryType(VkPhysicalDevice physicalDevice, uint32_t typeFilter, VkMemoryPropertyFlags properties) {
        VkPhysicalDeviceMemoryProperties memProperties;
        vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

        for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
                if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
                        return i;
                }
        }

        FATAL("Failed to find suitable memory type!");
}
void VulkanApplication::createBuffer(
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
void VulkanApplication::copyBuffer(
        VkDevice device,
        VkCommandPool commandPool,
        VkQueue queue,
        VkBuffer srcBuffer,
        VkBuffer dstBuffer,
        VkDeviceSize size
) {
        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandPool = commandPool;
        allocInfo.commandBufferCount = 1;

        // make a new command buffer for copying commands
        VkCommandBuffer commandBuffer;
        if (vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer) != VK_SUCCESS) {
                FATAL("Failed to allocate command buffer for buffer copy!");
        }

        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT; // only used once

        if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
                FATAL("Failed to begin recording command buffer for buffer copy!");
        }

        VkBufferCopy copyRegion{};
        copyRegion.srcOffset = 0;
        copyRegion.dstOffset = 0;
        copyRegion.size = size;
        vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

        if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
                FATAL("Failed to record command buffer for buffer copy!");
        }

        // execute the command buffer to finish copying
        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffer;

        // createa fence and wait for it to finish
        VkFenceCreateInfo fenceInfo{};
        fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        VkFence fence;
        if (vkCreateFence(device, &fenceInfo, nullptr, &fence) != VK_SUCCESS) {
                FATAL("Failed to create fence for buffer copy!");
        }

        if (vkQueueSubmit(queue, 1, &submitInfo, fence) != VK_SUCCESS) {
                FATAL("Failed to submit command buffer for buffer copy!");
        }

        vkWaitForFences(device, 1, &fence, VK_TRUE, std::numeric_limits<uint64_t>::max());
        vkDestroyFence(device, fence, nullptr);
        vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
}
std::pair<VkBuffer, VkDeviceMemory> VulkanApplication::createStagingBuffer(VulkanApplication* app, VkDeviceSize bufferSize) {
        VkBuffer stagingBuffer;
        VkDeviceMemory stagingBufferMemory;
        createBuffer(
                app->_physicalDevice,
                app->_logicalDevice,
                bufferSize,
                VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                stagingBuffer,
                stagingBufferMemory
        );
        return {stagingBuffer, stagingBufferMemory};
}