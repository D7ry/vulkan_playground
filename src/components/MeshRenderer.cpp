#include <glm/ext/matrix_transform.hpp>
#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>
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
void MeshRenderer::LoadModel(const char* meshFilePath) {
        tinyobj::attrib_t attrib;
        std::vector<tinyobj::shape_t> shapes;
        std::vector<tinyobj::material_t> materials;
        std::string warn, err;
        vertices.clear();
        indices.clear();

        if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, "../meshes/viking_room.obj")) {
                throw std::runtime_error(warn + err);
        }

        // deduplication
        std::unordered_map<Vertex, uint32_t> uniqueVertices{}; // unique vertex -> index

        for (const auto& shape : shapes) {
                for (const auto& index : shape.mesh.indices) {
                        Vertex vertex{};

                        vertex.pos
                                = {attrib.vertices[3 * index.vertex_index + 0],
                                   attrib.vertices[3 * index.vertex_index + 1],
                                   attrib.vertices[3 * index.vertex_index + 2]};

                        vertex.texCoord
                                = {attrib.texcoords[2 * index.texcoord_index + 0],
                                   1.0f - attrib.texcoords[2 * index.texcoord_index + 1]};

                        vertex.color = {1.0f, 1.0f, 1.0f};

                        if (uniqueVertices.find(vertex) == uniqueVertices.end()) {
                                uniqueVertices[vertex] = static_cast<uint32_t>(vertices.size());
                                vertices.push_back(vertex);
                        }

                        indices.push_back(uniqueVertices[vertex]);
                }
        }
}
void MeshRenderer::Rotate(float x, float y, float z) {
        transform.rotation.x += x;
        transform.rotation.y += y;
        transform.rotation.z += z;
}
