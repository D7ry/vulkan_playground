#pragma once
#include "ImGuiWidget.h"

class ImGuiWidgetDeviceInfo : public ImGuiWidget
{
  public:
    virtual void Draw(const VulkanEngine* engine) override {
        if (ImGui::Begin("Device info")) {
            ImGui::Text("Device Info");
            ImGui::End();
        }
    }
};
