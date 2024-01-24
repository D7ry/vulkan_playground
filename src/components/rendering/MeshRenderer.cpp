#include <glm/ext/matrix_transform.hpp>

#include "MeshRenderer.h"

glm::mat4 MeshRenderer::GetModelMatrix() {
    // calculate from transform's position, rotation, and scale
    auto position = transform.position;
    auto rotation = transform.rotation;
    auto scale = transform.scale;
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, position);
    model = glm::rotate(model, glm::radians(rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
    model = glm::rotate(model, glm::radians(rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
    model = glm::rotate(model, glm::radians(rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));
    model = glm::scale(model, scale);
    return model;
}

void MeshRenderer::Rotate(float x, float y, float z) {
    transform.rotation.x += x;
    transform.rotation.y += y;
    transform.rotation.z += z;
}
