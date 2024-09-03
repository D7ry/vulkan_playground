#pragma once
#include "ecs/Component.h"


struct PhongMesh;
struct PhongMeshInstanceComponent : IComponent
{
    PhongMesh* mesh;
    size_t dynamicUBOId; // id, id * sizeof(dynamicUBO) = offset of its own dynamic UBO
    int textureOffset; // index into the texture array
};
