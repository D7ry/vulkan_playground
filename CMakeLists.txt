cmake_minimum_required(VERSION 3.10)

project(vulkan_playground)
set(CMAKE_CXX_STANDARD 17)

find_package(Vulkan REQUIRED)

# spdlog
set(spdlog_repo /media/d7ry/TY7/dev/libs/linux_x86_64/spdlog)
set(spdlog_DIR ${spdlog_repo}/build)
find_package(spdlog REQUIRED)
# spdlog headers
set(spdlog_headers ${spdlog_repo}/include)

add_executable(vulkan_playground 
        src/main.cpp
        src/application/VulkanApplication.cpp
)

target_include_directories(vulkan_playground PRIVATE ${spdlog_headers})

target_link_libraries(vulkan_playground Vulkan::Vulkan glfw dl pthread X11 Xxf86vm Xrandr Xi spdlog::spdlog)

target_precompile_headers(vulkan_playground PUBLIC src/PCH.h)