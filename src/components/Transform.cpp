#include "Transform.h"
#include "imgui.h"

void Transform::DrawImguiControllers() {
        // Position controller
        ImGui::Text("Position");
        ImGui::SliderFloat("X##Position", &position.x, -10.0f, 10.0f);
        ImGui::SliderFloat("Y##Position", &position.y, -10.0f, 10.0f);
        ImGui::SliderFloat("Z##Position", &position.z, -10.0f, 10.0f);

        // Rotation controller
        ImGui::Text("Rotation");
        ImGui::SliderFloat("X##Rotation", &rotation.x, -180.0f, 180.0f);
        ImGui::SliderFloat("Y##Rotation", &rotation.y, -180.0f, 180.0f);
        ImGui::SliderFloat("Z##Rotation", &rotation.z, -180.0f, 180.0f);

        // Scale controller
        ImGui::Text("Scale");
        ImGui::SliderFloat("X##Scale", &scale.x, 0.0f, 10.0f);
        ImGui::SliderFloat("Y##Scale", &scale.y, 0.0f, 10.0f);
        ImGui::SliderFloat("Z##Scale", &scale.z, 0.0f, 10.0f);
}
