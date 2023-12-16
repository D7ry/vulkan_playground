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

add_executable(vulkan_playground 
        src/main.cpp
        src/application/VulkanApplication.cpp
        src/utils/Logging.cpp
)
target_include_directories(vulkan_playground PUBLIC src)
target_include_directories(vulkan_playground PRIVATE spdlog_headers)

target_precompile_headers(vulkan_playground PUBLIC src/PCH.h)
target_link_libraries(vulkan_playground Vulkan::Vulkan glfw dl pthread X11 Xxf86vm Xrandr Xi spdlog::spdlog)

