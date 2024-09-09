#pragma once

#include "structs/SharedEngineStructs.h"

#include "Component.h"
#include "Entity.h"

class ISystem
{
  public:
    ISystem() {}

    virtual void Init(const InitContext* initData) = 0;

    // iterate through all its nodes, and perform update logic on the nodes'
    // data
    virtual void Tick(const TickContext* tickData) = 0;

    virtual void Cleanup() = 0;

    virtual ~ISystem() {}

    virtual void AddEntity(Entity* entity) { _entities.push_back(entity); }

    // TODO: handle entity removal
    // may need to make entity point to systems

  protected:
    std::vector<Entity*> _entities;
};

// System that supports ImGui drawing
// Overload DrawImGui() to draw imgui elements
class ImGuiSystem : public ISystem
{
  public:
    virtual void DrawImGui() = 0;
};

// System that works as-is and does not need entity at all.
class ISingletonSystem : public ISystem
{
    virtual void AddEntity(Entity* entity) override {
        FATAL("Singleton systems cannot have entities");
    }
};

// system that belongs to the graphics render pass.
// each system has its own pipeline and own render logic
class IRenderSystem : public ISystem
{
  protected:
};
