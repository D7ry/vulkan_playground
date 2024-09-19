#include "VulkanEngine.h"

#include "ImGuiWidget.h"

// current setup assumes single logical device
void ImGuiWidgetDeviceInfo::Draw(const VulkanEngine* engine) {
    VQDevice* device = engine->_device.get();
    auto properties = device->properties;
    ImGui::SeparatorText("General");
    ImGui::Text("%s", device->properties.deviceName);
    ImGui::SeparatorText("Device Extensions");
    ImGui::Indent(10);
    for (const char* extension : engine->DEVICE_EXTENSIONS) {
        ImGui::Text("%s", extension);
    }
    ImGui::Indent(-10);
}
