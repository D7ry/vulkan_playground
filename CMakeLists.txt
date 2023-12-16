cmake_minimum_required(VERSION 3.10)

project(vulkan_playground)
set(CMAKE_CXX_STANDARD 17)

find_package(Vulkan REQUIRED)

add_executable(vulkan_playground 
        src/main.cpp
        src/application/VulkanApplication.cpp
)

target_link_libraries(vulkan_playground Vulkan::Vulkan glfw dl pthread X11 Xxf86vm Xrandr Xi)
