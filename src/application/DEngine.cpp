#include "DEngine.h"

void DEngine::addExperimentalMesh() {
    const std::string SAMPLE_TEXTURE_PATH
        = "../resources/textures/viking_room.png";
    const std::string SAMPLE_MESH_PATH = "../resources/meshes/viking_room.obj";
    // MeshInstance* render3 =

    //     render3->transform.position = glm::vec3(0, 2, 2);

    // // _vulkanEngine->AddMesh(render);
    // // _vulkanEngine->AddMesh(render2);
    // _vulkanEngine->AddMesh(render3);
    //_vulkanEngine->AddMesh(render4);
}

void DEngine::Init() {
    INFO("Initializing dEngine...");
    initGLFW();
    { // render manager
        _vulkanEngine = std::make_unique<VulkanEngine>();
        _vulkanEngine->Init(_window);
        _vulkanEngine->SetImguiRenderCallback([this]() { this->renderImgui(); }
        );
        addExperimentalMesh();
        _vulkanEngine->Prepare();
    }
    { // input manager
        this->_inputManager = std::make_unique<InputManager>();
        auto keyCallback = [](GLFWwindow* window,
                              int key,
                              int scancode,
                              int action,
                              int mods) {
            auto app
                = reinterpret_cast<DEngine*>(glfwGetWindowUserPointer(window));
            app->_inputManager->OnKeyInput(window, key, scancode, action, mods);
            ImGui_ImplGlfw_KeyCallback(window, key, scancode, action, mods);
        };
        glfwSetKeyCallback(this->_window, keyCallback);
        this->bindInputs(); // register key callbacks
    }
    { // don't have a mouse input manager yet, so manually bind cursor pos
      // callback
        auto cursorPosCallback = [](GLFWwindow* window, double xpos, double ypos
                                 ) {
            auto app
                = reinterpret_cast<DEngine*>(glfwGetWindowUserPointer(window));
            app->cursorPosCallback(window, xpos, ypos);
            ImGui_ImplGlfw_CursorPosCallback(window, xpos, ypos);
        };
        glfwSetCursorPosCallback(this->_window, cursorPosCallback);
    }
}

void DEngine::Run() {
    while (!glfwWindowShouldClose(_window)) {
        Tick();
    }
    cleanup();
}

void DEngine::Tick() {
    glfwPollEvents();
    _deltaTimer.Tick();
    float deltaTime = _deltaTimer.GetDeltaTime();
    _vulkanEngine->SetViewMatrix(_mainCamera.GetViewMatrix()
    ); // TODO: only update the view matrix when updating the camera
    _vulkanEngine->Tick(deltaTime);
    _inputManager->Tick(deltaTime);
};

void DEngine::initGLFW() {
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

void DEngine::renderImgui() {
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
