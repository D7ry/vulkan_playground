#pragma once
#include "ecs/Component.h"

class BindlessRenderSystem;
struct BindlessRenderSystemComponent : IComponent
{
    void FlagUpdate();
    BindlessRenderSystem* parentSystem;
    int instanceDataOffset;
};
