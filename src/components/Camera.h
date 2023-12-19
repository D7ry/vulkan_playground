#pragma once
#include <glm/glm.hpp>
/**
 * @brief Camera are meant to be used with Vulkan Applications.
 * Takes some form of input and translates/rotates the camera position.
 *
 */
class Camera {
      public:
        Camera(glm::vec3 position, glm::vec3 rotation);
        glm::mat4 GetViewMatrix();  
        void ModRotation(float yaw, float pitch, float roll);
        void ModPosition(float x, float y, float z);

      private:
        glm::vec3 _position; // world position of the camera
        glm::vec3 _rotation; // pitch, yaw, roll
};