#pragma once
#include "System.h"

class SystemManager
{
  public:
    enum class SystemType
    {
        ComputeSystem,
        RenderSystem
    };

    void AddSystem(ISystem* system) { _renderSystems.push_back(system); }

    void TickRender(const TickData* tickData) {
        for (ISystem* system : _renderSystems) {
            system->Tick(tickData);
        }
    }

  private:
    std::vector<ISystem*> _renderSystems;
    std::vector<ISystem*> _computeSystems;
};
