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
 * @brief Base class for all Vulkan applications.
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
        void initVulkan();
        void mainLoop();
        GLFWwindow* _window;

        // validation layers, enable under debug mode only
#ifdef NDEBUG
        const bool enableValidationLayers = false;
#else
        const bool enableValidationLayers = true;
#endif
        static inline const std::vector<const char*> VALIDATION_LAYERS = {
            "VK_LAYER_KHRONOS_validation"};

        /**
         * @brief Check if all requested validation layers are available.
         * 
         * @return true if all requested validation layers are available.
         * @return false if any requested validation layer is not available.
         */
        bool checkValidationLayerSupport();
};
