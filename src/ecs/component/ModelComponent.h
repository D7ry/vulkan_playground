#pragma once
#include "lib/VQBuffer.h"
#include "ecs/Component.h"

class ModelComponent : public IComponent
{
    VQBuffer vertexBuffer;
    VQBufferIndex indexBuffer;
};
