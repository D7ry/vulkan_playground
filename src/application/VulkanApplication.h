#pragma once
#include <vulkan/vulkan.h>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <limits>
#include <optional>
#include <set>
#include <stdexcept>
#include <vector>

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

      private:
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

        // validation layers, enable under debug mode only
#ifdef NDEBUG
        const bool enableValidationLayers = false;
#else
        const bool enableValidationLayers = true;
#endif
        static inline const std::vector<const char*> VALIDATION_LAYERS = {"VK_LAYER_KHRONOS_validation"};

        bool checkValidationLayerSupport();

        // debug messenger setup
        static VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
                                                     const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger);
        void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);
        void setupDebugMessenger();
        static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                                                            VkDebugUtilsMessageTypeFlagsEXT messageType,
                                                            const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData);

        // private fields
        static inline const char* APPLICATION_NAME = "Vulkan Application";
        static inline const uint32_t APPLICATION_VERSION = VK_MAKE_VERSION(1, 0, 0);
        static inline const char* ENGINE_NAME = "No Engine";
        static inline const uint32_t ENGINE_VERSION = VK_MAKE_VERSION(1, 0, 0);
        static inline const uint32_t API_VERSION = VK_API_VERSION_1_0;
        
        GLFWwindow* _window;
        VkInstance _instance;
        VkSurfaceKHR _surface;
        VkDebugUtilsMessengerEXT _debugMessenger;
};
