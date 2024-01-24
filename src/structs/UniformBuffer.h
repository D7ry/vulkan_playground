#pragma once
#include <glm/glm.hpp>

struct UniformBuffer
{
};

struct UniformBuffer_Static : UniformBuffer
{
    glm::mat4 view;
    glm::mat4 proj;
};

struct UniformBuffer_Dynamic : UniformBuffer
{
    glm::mat4 model;
};