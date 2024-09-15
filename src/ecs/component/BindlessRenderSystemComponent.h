#pragma once
#include "ecs/Component.h"

class BindlessRenderSystem;

struct BindlessRenderSystemComponent : IComponent
{
    friend BindlessRenderSystem;
    void FlagUpdate();
  private:
    BindlessRenderSystemComponent() = default;
    BindlessRenderSystem* parentSystem;
    int instanceDataOffset; // used to update the instance data
};
