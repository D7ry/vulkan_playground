#include "PhongRenderSystemInstancedComponent.h"
#include "ecs/system/PhongRenderSystemInstanced.h"

void PhongRenderSystemInstancedComponent::FlagAsDirty(Entity* owner) {
    ASSERT(owner->GetComponent<PhongRenderSystemInstancedComponent>() == this)
    parentSystem->FlagAsDirty(owner);
}
