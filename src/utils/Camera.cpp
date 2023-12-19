#include "Camera.h"
#include <glm/gtc/matrix_transform.hpp>

Camera::Camera(glm::vec3 position, glm::vec3 rotation) {
        this->_position = position;
        this->_rotation = rotation;
};

void Camera::ModRotation(float yaw, float pitch, float roll) {
        this->_rotation.y = fmod(this->_rotation.y + yaw, 360.0f);
        this->_rotation.x = fmod(this->_rotation.x + pitch, 360.0f);
        this->_rotation.z = fmod(this->_rotation.z + roll, 360.0f);
};


glm::mat4 Camera::GetViewMatrix() {
                // Convert angles (assumed to be in degrees) to radians
        glm::vec3 anglesRad = glm::radians(_rotation);

        // Calculate direction vector
        glm::vec3 direction;
        direction.x = cos(anglesRad.y) * cos(anglesRad.x);
        direction.y = sin(anglesRad.y) * cos(anglesRad.x);
        direction.z = sin(anglesRad.x);

        // Normalize the direction
        direction = glm::normalize(direction);

        // Calculate right and up vectors
        const glm::vec3 worldUp = glm::vec3(0, 0, 1);
        glm::vec3 right = glm::normalize(glm::cross(worldUp, direction));
        glm::vec3 up = glm::cross(direction, right);

            // Include roll in the up vector calculation
        glm::mat4 rollMatrix = glm::rotate(glm::mat4(1.0f), glm::radians(_rotation.z), direction);
        up = glm::vec3(rollMatrix * glm::vec4(up, 0.0f));


        INFO("Camera position: ({}, {}, {})", _position.x, _position.y, _position.z);
        INFO("Camera direction: ({}, {}, {})", direction.x, direction.y, direction.z);

        // Create view matrix
        return glm::lookAt(_position, _position + direction, up);

 };