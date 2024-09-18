#pragma once
#include "imgui.h"

class VulkanEngine;

// An ImGui widget that draws information related to 
// the main engine.
class ImGuiWidget
{
  public:
    ImGuiWidget() {};
    virtual void Draw(const VulkanEngine* engine) = 0;
};
