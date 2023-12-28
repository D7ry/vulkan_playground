#pragma once
#include "Transform.h"
#include "structs/Vertex.h"
#include <glm/ext/matrix_transform.hpp>
#include <vulkan/vulkan_core.h>

#include "MeshRenderManager.h"
class MeshRenderer {
      public:
        MeshRenderer() = delete;
        MeshRenderer(const char* meshFilePath, const char* textureFilePath) {
                this->meshFilePath = meshFilePath;
                this->textureFilePath = textureFilePath;
        }

        void RegisterRenderManager(MeshRenderManager* manager) {
                manager->RegisterMeshRenderer(this, MeshRenderManager::RenderMethod::Generic);
        }
        void Rotate(float x, float y, float z);
        ~MeshRenderer();
        glm::mat4 GetModelMatrix();
        void Update();
        void Render();

        std::vector<Vertex> vertices;
        std::vector<uint32_t> indices;

        Transform transform = Transform{{0, 0, 0}, {0, 0, 0}, {1, 1, 1}};

        // transform
        bool isVisible = true;
        std::string meshFilePath;
        std::string textureFilePath;
};