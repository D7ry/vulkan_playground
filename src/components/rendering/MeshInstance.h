#pragma once
#include "components/Transform.h"
#include "imgui.h"
#include "structs/Vertex.h"
#include <glm/ext/matrix_transform.hpp>
#include <vulkan/vulkan_core.h>

#include "MeshRenderManager.h"

/**
 * @brief One instance of a mesh to be rendered.
 *
 */
class MeshInstance
{
  public:
    MeshInstance() = delete;

    MeshInstance(const char* meshFilePath, const char* textureFilePath) {
        this->meshFilePath = meshFilePath;
        this->textureFilePath = textureFilePath;
    }

    void Rotate(float x, float y, float z);
    ~MeshInstance();
    glm::mat4 GetModelMatrix();
    void Update();
    void Render();

    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;

    Transform transform = Transform{{0, 0, 0}, {0, 0, 0}, {1, 1, 1}};

    void DrawImguiController() {
        ImGui::Separator();
        ImGui::Text("Mesh: %s", this->meshFilePath.data());
        ImGui::Text("Texture: %s", this->textureFilePath.data());
        if (ImGui::BeginChild(
                fmt::format("{}", (void*)this).data(),
                ImVec2(ImGui::GetWindowWidth() * 0.9, ImGui::GetWindowHeight() * 0.2),
                0
            )) {
            transform.DrawImguiControllers();
        }
        ImGui::EndChild();
    }

    // transform
    bool isVisible = true;
    std::string meshFilePath;
    std::string textureFilePath;
};
