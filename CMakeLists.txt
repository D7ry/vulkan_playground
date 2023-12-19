cmake_minimum_required(VERSION 3.22)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

project(vulkan_playground)
set(CMAKE_CXX_STANDARD 17)

find_package(Vulkan REQUIRED)

# spdlog
set(spdlog_repo /home/d7ry/lib/linux_x86_64/spdlog)
set(spdlog_DIR ${spdlog_repo}/build)
find_package(spdlog REQUIRED)
# spdlog headers
set(spdlog_headers ${spdlog_repo}/include/spdlog)

#imgui
set(IMGUI_DIR /home/d7ry/lib/linux_x86_64/imgui)
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
        src/application/TriangleApp.cpp
        src/components/Logging.cpp
        src/components/ShaderUtils.cpp
        src/components/Animation.cpp
        src/components/DeltaTimer.cpp
        src/components/Camera.cpp
        src/structs/Vertex.cpp
        src/components/VulkanUtils.cpp
        src/components/ImGuiManager.cpp
)
target_include_directories(vulkan_playground PUBLIC src)
target_include_directories(vulkan_playground PRIVATE spdlog_headers)


target_precompile_headers(vulkan_playground PUBLIC src/PCH.h)
target_link_libraries(vulkan_playground Vulkan::Vulkan glfw dl pthread X11 Xxf86vm Xrandr Xi spdlog::spdlog imgui)

