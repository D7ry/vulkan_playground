#pragma once
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

class DepthBuffer {
        public:
        
        private:
        VkImage _image;
        VkImageView _imaveView;
        VkDeviceMemory _memory;
};