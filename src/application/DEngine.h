#pragma once

#include "components/Camera.h"
#include "components/DeltaTimer.h"
#include "components/InputManager.h"
#include "components/rendering/VulkanEngine.h"

#include "imgui.h"
#include <GLFW/glfw3.h>
#include <memory>

class DEngine
{
  public:
    DEngine(){};

    void Init();

    /**
     * @brief Run the engine in the main loop. 
     * The engine will keep ticking until exception, or window closes.
     */
    void Run();


  private:
    // main update function
    void tick();
    
    DeltaTimer _deltaTimer;
    Camera _mainCamera = Camera(0, 0, 0, 0, 0, 0);

    GLFWwindow* _window;
    std::unique_ptr<VulkanEngine> _vulkanEngine;
    std::unique_ptr<InputManager> _inputManager;

    bool _lockCursor;

    void cursorPosCallback(GLFWwindow* window, double xpos, double ypos) {
        static bool updatedCursor = false;
        static int prevX = -1;
        static int prevY = -1;
        if (!updatedCursor) {
            prevX = xpos;
            prevY = ypos;
            updatedCursor = true;
        }
        double deltaX = prevX - xpos;
        double deltaY = prevY - ypos;
        // handle camera movement
        deltaX *= 0.3;
        deltaY *= 0.3; // make movement slower
        if (_lockCursor) {
            _mainCamera.ModRotation(deltaX, deltaY, 0);
        }
        prevX = xpos;
        prevY = ypos;
    }

    void initGLFW();

    void renderImgui();

    void cleanup() {
        _vulkanEngine->Cleanup();
        glfwDestroyWindow(this->_window);
        glfwTerminate();
    }

    void bindInputs() {
        static const int CAMERA_SPEED = 3;
        _inputManager->RegisterCallback(GLFW_KEY_W, InputManager::KeyCallbackCondition::HOLD, [this]() {
            _mainCamera.Move(_deltaTimer.GetDeltaTime() * CAMERA_SPEED, 0, 0);
        });
        _inputManager->RegisterCallback(GLFW_KEY_S, InputManager::KeyCallbackCondition::HOLD, [this]() {
            _mainCamera.Move(-_deltaTimer.GetDeltaTime() * CAMERA_SPEED, 0, 0);
        });
        _inputManager->RegisterCallback(GLFW_KEY_A, InputManager::KeyCallbackCondition::HOLD, [this]() {
            _mainCamera.Move(0, _deltaTimer.GetDeltaTime() * CAMERA_SPEED, 0);
        });
        _inputManager->RegisterCallback(GLFW_KEY_D, InputManager::KeyCallbackCondition::HOLD, [this]() {
            _mainCamera.Move(0, -_deltaTimer.GetDeltaTime() * CAMERA_SPEED, 0);
        });
        _inputManager->RegisterCallback(GLFW_KEY_LEFT_CONTROL, InputManager::KeyCallbackCondition::HOLD, [this]() {
            _mainCamera.Move(0, 0, -CAMERA_SPEED * _deltaTimer.GetDeltaTime());
        });
        _inputManager->RegisterCallback(GLFW_KEY_SPACE, InputManager::KeyCallbackCondition::HOLD, [this]() {
            _mainCamera.Move(0, 0, CAMERA_SPEED * _deltaTimer.GetDeltaTime());
        });

        _inputManager->RegisterCallback(GLFW_KEY_TAB, InputManager::KeyCallbackCondition::PRESS, [this]() {
            _lockCursor = !_lockCursor;
            if (_lockCursor) {
                glfwSetInputMode(_window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
                ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_NoMouse;
            } else {
                glfwSetInputMode(_window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
                ImGui::GetIO().ConfigFlags &= ~ImGuiConfigFlags_NoMouse;
            }
        });
        _inputManager->RegisterCallback(GLFW_KEY_ESCAPE, InputManager::KeyCallbackCondition::PRESS, [this]() {
            glfwSetWindowShouldClose(_window, GLFW_TRUE);
        });
    }
};
