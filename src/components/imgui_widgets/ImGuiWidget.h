#pragma once
#include "imgui.h"

class VulkanEngine;

class ImGuiWidget
{
  public:
    ImGuiWidget() {};
    virtual void Draw(const VulkanEngine* engine) = 0;
};
