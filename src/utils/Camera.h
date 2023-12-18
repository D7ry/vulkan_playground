#pragma once
#include <glm/glm.hpp>
/**
 * @brief Camera are meant to be used with Vulkan Applications.
 * Takes some form of input and translates/rotates the camera position.
 *
 */
class Camera {
      public:
        Camera(){

        };
        glm::vec4 GetViewMatrix() {

        };
      private:
        glm::mat4 _position; // world position of the camera
        glm::vec3 _rotation; // yaw, pitch, roll
};