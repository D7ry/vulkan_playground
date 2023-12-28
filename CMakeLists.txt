cmake_minimum_required(VERSION 3.22)
set(CMAKE_C_COMPILER "/usr/bin/clang")
set(CMAKE_CXX_COMPILER "/usr/bin/clang++")

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(spdlog_DIR /home/d7ry/lib/linux_x86_64/spdlog/build)
set(IMGUI_DIR /home/d7ry/lib/linux_x86_64/imgui)
set(STB_DIR /home/d7ry/lib/common/stb)
set(TinyObjectLoader_DIR /home/d7ry/lib/common/tinyobjloader)

project(vulkan_playground)
set(CMAKE_CXX_STANDARD 17)

find_package(Vulkan REQUIRED)

# spdlog
find_package(spdlog REQUIRED)

#imgui
add_library(
        imgui
        ${IMGUI_DIR}/imgui.cpp
        ${IMGUI_DIR}/imgui_demo.cpp
        ${IMGUI_DIR}/imgui_draw.cpp
        ${IMGUI_DIR}/imgui_widgets.cpp
        ${IMGUI_DIR}/imgui_tables.cpp
        ${IMGUI_DIR}/backends/imgui_impl_glfw.cpp
        ${IMGUI_DIR}/backends/imgui_impl_vulkan.cpp
)
target_include_directories(imgui PUBLIC ${IMGUI_DIR} ${IMGUI_DIR}/backends)

add_executable(vulkan_playground 
        src/main.cpp
        src/application/VulkanApplication.cpp
        src/components/Logging.cpp
        src/components/ShaderUtils.cpp
        src/components/Animation.cpp
        src/components/DeltaTimer.cpp
        src/components/Camera.cpp
        src/structs/Vertex.cpp
        src/components/VulkanUtils.cpp
        src/components/ImGuiManager.cpp
        src/components/TextureManager.cpp
        src/components/rendering/MetaPipeline.cpp
        src/components/rendering/MeshRenderer.cpp
        src/components/rendering/MeshRenderManager.cpp
        src/components/InputManager.cpp
        src/lib/VQDevice.cpp
        src/lib/VQUtils.cpp
        src/components/rendering/RenderManager.cpp
        src/application/DEngine.cpp

        src/components/rendering/RenderManager.cpp
)
# std directory
target_include_directories(vulkan_playground PUBLIC ${STB_DIR})
target_include_directories(vulkan_playground PUBLIC ${TinyObjectLoader_DIR})
target_include_directories(vulkan_playground PUBLIC src)


target_precompile_headers(vulkan_playground PUBLIC src/PCH.h)
target_link_libraries(vulkan_playground Vulkan::Vulkan glfw dl pthread X11 Xxf86vm Xrandr Xi spdlog::spdlog imgui)

