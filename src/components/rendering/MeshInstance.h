#pragma once
#include "components/Transform.h"
#include "components/rendering/RenderGroup.h"
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
    friend class RenderGroup;
    friend class MeshRenderManager;
    void Rotate(float x, float y, float z);
    ~MeshInstance();
    glm::mat4 GetModelMatrix();
    void Update();
    void Render();

    Transform transform = Transform{{0, 0, 0}, {0, 0, 0}, {1, 1, 1}};

    void DrawImguiController() {
        ImGui::Separator();
        if (ImGui::BeginChild(
                fmt::format("{}", (void*)this).data(),
                ImVec2(
                    ImGui::GetWindowWidth() * 0.9,
                    ImGui::GetWindowHeight() * 0.2
                ),
                0
            )) {
            transform.DrawImguiControllers();
        }
        ImGui::EndChild();
    }

    // transform
    bool isVisible = true;

  private:
    MeshInstance(){}; // default constructor is hidden from public
    uint32_t _index;  // index of the instance in the renderGroup.
};
