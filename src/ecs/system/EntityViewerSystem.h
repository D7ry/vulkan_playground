#pragma once

#include "ecs/System.h"

// show a list of entities and their components
class EntityViewerSystem : public ImGuiSystem
{
  public:
    virtual void DrawImGui() override;

    virtual void Init(const InitContext* initData) override {};

    // iterate through all its nodes, and perform update logic on the nodes'
    // data
    virtual void Tick(const TickContext* tickData) override {};
    virtual void Cleanup() override{};
  private:
    void drawEntity(Entity* entity);
};
