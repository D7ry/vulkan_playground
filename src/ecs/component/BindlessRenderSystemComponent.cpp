#include "ecs/system/BindlessRenderSystem.h"
#include "BindlessRenderSystemComponent.h"

void BindlessRenderSystemComponent::FlagUpdate() {
    // pushe the owner of this component to parent's update queue
    this->parentSystem->FlagUpdate(this->parent);
}
