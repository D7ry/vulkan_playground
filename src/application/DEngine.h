#pragma once

#include "components/Camera.h"
#include "components/DeltaTimer.h"
#include "components/InputManager.h"
#include "components/rendering/VulkanEngine.h"

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include <GLFW/glfw3.h>
#include <memory>

class DEngine
{
      public:
        DEngine(){};

        void addExperimentalMesh()
        {
                const std::string SAMPLE_TEXTURE_PATH = "../resources/textures/viking_room.png";
                const std::string SAMPLE_MESH_PATH = "../resources/meshes/viking_room.obj";
                MeshRenderer* render = new MeshRenderer(SAMPLE_MESH_PATH.data(), SAMPLE_TEXTURE_PATH.data());
                MeshRenderer* render2 = new MeshRenderer(SAMPLE_MESH_PATH.data(), SAMPLE_TEXTURE_PATH.data());
                MeshRenderer* render3
                        = new MeshRenderer("../resources/meshes/spot.obj", "../resources/textures/spot.png");
                MeshRenderer* render4
                        = new MeshRenderer("../resources/meshes/wall.obj", "../resources/textures/wall.png");
                render2->transform.position = glm::vec3(0, 0, 2);

                render3->transform.position = glm::vec3(0, 2, 2);

                render3->transform.position = glm::vec3(0, 2, 2);

                render4->transform.position = glm::vec3(0, 0, -10);
                render4->transform.rotation = {0, 0, 180};

                _vulkanEngine->AddMesh(render);
                _vulkanEngine->AddMesh(render2);
                _vulkanEngine->AddMesh(render3);
                _vulkanEngine->AddMesh(render4);
        }

        void Init()
        {
                INFO("Initializing dEngine...");
                initGLFW();
                { // render manager
                        _vulkanEngine = std::make_unique<VulkanEngine>();
                        _vulkanEngine->Init(_window);
                        _vulkanEngine->SetImguiRenderCallback([this]() { this->renderImgui(); });
                        addExperimentalMesh();
                        _vulkanEngine->Prepare();
                }
                { // input manager
                        this->_inputManager = std::make_unique<InputManager>();
                        auto keyCallback = [](GLFWwindow* window, int key, int scancode, int action, int mods) {
                                auto app = reinterpret_cast<DEngine*>(glfwGetWindowUserPointer(window));
                                app->_inputManager->OnKeyInput(window, key, scancode, action, mods);
                                ImGui_ImplGlfw_KeyCallback(window, key, scancode, action, mods);
                        };
                        glfwSetKeyCallback(this->_window, keyCallback);
                        this->bindInputs(); // register key callbacks
                }
                { // don't have a mouse input manager yet, so manually bind cursor pos callback
                        auto cursorPosCallback = [](GLFWwindow* window, double xpos, double ypos) {
                                auto app = reinterpret_cast<DEngine*>(glfwGetWindowUserPointer(window));
                                app->cursorPosCallback(window, xpos, ypos);
                                ImGui_ImplGlfw_CursorPosCallback(window, xpos, ypos);
                        };
                        glfwSetCursorPosCallback(this->_window, cursorPosCallback);
                }
        }

        void Run()
        {
                while (!glfwWindowShouldClose(_window)) {
                        Tick();
                }
                cleanup();
        }

        // main update function
        void Tick()
        {
                glfwPollEvents();
                _deltaTimer.Tick();
                float deltaTime = _deltaTimer.GetDeltaTime();
                _vulkanEngine->SetViewMatrix(_mainCamera.GetViewMatrix()
                ); // TODO: only update the view matrix when updating the camera
                _vulkanEngine->Tick(deltaTime);
                _inputManager->Tick(deltaTime);
        };

      private:
        DeltaTimer _deltaTimer;
        Camera _mainCamera = Camera(0, 0, 0, 0, 0, 0);

        GLFWwindow* _window;
        std::unique_ptr<VulkanEngine> _vulkanEngine;
        std::unique_ptr<InputManager> _inputManager;

        bool _lockCursor;

        void cursorPosCallback(GLFWwindow* window, double xpos, double ypos)
        {
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

        void initGLFW()
        {
                glfwInit();
                glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
                glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
                if (!glfwVulkanSupported()) {
                        FATAL("Vulkan is not supported on this machine!");
                }
                this->_window = glfwCreateWindow(1280, 720, "DEngine", nullptr, nullptr);
                if (this->_window == nullptr) {
                        FATAL("Failed to initialize GLFW windlw!");
                }
                glfwSetWindowUserPointer(_window, this);
        }

        void renderImgui()
        {
                if (ImGui::Begin("Debug")) {
                        ImGui::SetWindowPos(ImVec2(0, 0), ImGuiCond_Once);
                        ImGui::SetWindowSize(ImVec2(400, 400), ImGuiCond_Once);
                        ImGui::Text("Framerate: %f", 1 / _deltaTimer.GetDeltaTime());
                        ImGui::Separator();
                        ImGui::Text("Camera");
                        if (ImGui::BeginChild("Camera")) {
                                ImGui::Text(
                                        "Position: (%f, %f, %f)",
                                        _mainCamera.GetPosition().x,
                                        _mainCamera.GetPosition().y,
                                        _mainCamera.GetPosition().z
                                );
                                ImGui::Text(
                                        "Yaw: %f Pitch: %f Roll: %f",
                                        _mainCamera.GetRotation().y,
                                        _mainCamera.GetRotation().x,
                                        _mainCamera.GetRotation().z
                                );
                                if (_lockCursor) {
                                        ImGui::Text("View Mode: Active");
                                } else {
                                        ImGui::Text("View Mode: Deactive");
                                }
                        }
                        ImGui::EndChild();
                }
                ImGui::End();
                _vulkanEngine->DrawImgui();
        }

        void cleanup()
        {
                _vulkanEngine->Cleanup();
                glfwDestroyWindow(this->_window);
                glfwTerminate();
        }

        void bindInputs()
        {
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
                _inputManager->RegisterCallback(
                        GLFW_KEY_LEFT_CONTROL,
                        InputManager::KeyCallbackCondition::HOLD,
                        [this]() { _mainCamera.Move(0, 0, -CAMERA_SPEED * _deltaTimer.GetDeltaTime()); }
                );
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