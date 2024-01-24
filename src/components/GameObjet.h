#pragma once
#include "MeshRenderer.h"
#include "components/Transform.h"
#include <glm/glm.hpp>

class GameObject
{
  public:
    GameObject();
    ~GameObject();
    void Update();
    void Render();

  private:
    // transform
    Transform _transform;

    MeshRenderer _meshRenderer; // note that mesh renderer uses its own transform
};