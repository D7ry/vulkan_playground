#pragma once
#include "Transform.h"
#include "structs/Vertex.h"
#include <glm/ext/matrix_transform.hpp>
#include <vulkan/vulkan_core.h>

#include "MeshRenderManager.h"
class MeshRenderer {
      public:
        MeshRenderer() {};

        void LoadModel(const char* meshFilePath);
        void RegisterRenderManager(MeshRenderManager* manager) {
                manager->RegisterMeshRenderer(this, MeshRenderManager::RenderPipeline::Generic);
        }
        ~MeshRenderer();
        glm::mat4 GetModelMatrix();
        void Update();
        void Render();

        std::vector<Vertex> vertices;
        std::vector<uint32_t> indices;

        Transform transform = Transform{{0, 0, 0},{0, 0, 0}, {1, 1, 1}};

      private:
        // transform
        bool _isVisible = true;

};