#include "VulkanApplication.h"
#include <GLFW/glfw3.h>

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
void VulkanApplication::initVulkan() {
        INFO("Initializing Vulkan...");
        if (this->enableValidationLayers) {
                if ( !this->checkValidationLayerSupport()) {
                        ERROR("Validation layers requested, but not available!");
                } else {
                        INFO("Validation layers requested, and available.");
                }
        }
        INFO("Vulkan initialized.");

}
