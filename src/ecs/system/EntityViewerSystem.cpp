#include "EntityViewerSystem.h"
#include "ecs/component/PhongRenderSystemInstancedComponent.h"
#include "ecs/component/TransformComponent.h"
#include "imgui.h"

void EntityViewerSystem::DrawImGui() {
    ImGui::Begin("Entity Viewer");
    int i = 1; // index for entity
    for (Entity* entity : _entities) {
        ImGui::BeginChild(
            i,
            ImVec2(
                ImGui::GetWindowWidth() * 0.9, ImGui::GetWindowHeight() * 0.3
            )
        );
        drawEntity(entity);
        ImGui::EndChild();

        i++;
    }
    ImGui::End();
}

void EntityViewerSystem::drawEntity(Entity* entity) {
    ImGui::SeparatorText(entity->GetName());

    // TransformComponent
    if (TransformComponent* transform
        = entity->GetComponent<TransformComponent>()) {
        ImGui::Text("Position");
        bool changed = false;

        changed |= ImGui::SliderFloat(
            "X##Position", &transform->position.x, -10.0f, 10.0f
        );
        changed |= ImGui::SliderFloat(
            "Y##Position", &transform->position.y, -10.0f, 10.0f
        );
        changed |= ImGui::SliderFloat(
            "Z##Position", &transform->position.z, -10.0f, 10.0f
        );

        // Rotation controller
        ImGui::Text("Rotation");
        changed |= ImGui::SliderFloat(
            "X##Rotation", &transform->rotation.x, -180.0f, 180.0f
        );
        changed |= ImGui::SliderFloat(
            "Y##Rotation", &transform->rotation.y, -180.0f, 180.0f
        );
        changed |= ImGui::SliderFloat(
            "Z##Rotation", &transform->rotation.z, -180.0f, 180.0f
        );

        // Scale controller
        ImGui::Text("Scale");
        changed |= ImGui::SliderFloat("X##Scale", &transform->scale.x, 0.0f, 10.0f);
        changed |= ImGui::SliderFloat("Y##Scale", &transform->scale.y, 0.0f, 10.0f);
        changed |= ImGui::SliderFloat("Z##Scale", &transform->scale.z, 0.0f, 10.0f);

        // flush instance transform to device buffer
        if (changed) {
            if (PhongRenderSystemInstancedComponent* renderInstance = entity->GetComponent<PhongRenderSystemInstancedComponent>()){
                renderInstance->FlagAsDirty(entity);
            }
        }

    }
}
