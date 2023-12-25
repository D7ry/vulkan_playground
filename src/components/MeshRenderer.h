#pragma once
#include "Transform.h"
#include <vulkan/vulkan_core.h>
class MeshRenderer {
        public:
                MeshRenderer() = delete;

                
                ~MeshRenderer();
                void Update();
                void Render();
        private:
                // transform
                Transform _transform;
                VkPipeline _pipeline;
};