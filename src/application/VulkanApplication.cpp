#include "VulkanApplication.h"
#include <GLFW/glfw3.h>
#include <vulkan/vulkan_core.h>

void VulkanApplication::Run() {
        INFO("Initializing Vulkan Application...");
        this->initWindow();
        this->initVulkan();
        this->mainLoop();
        // cleanup();
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
                if ( !this->checkValidationLayerSupport()) {
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
        printf("setting up debug messenger .... \n");
        auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");

        if (func != nullptr) {
                return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
        } else {
                printf("function is nullptr!\n");
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
