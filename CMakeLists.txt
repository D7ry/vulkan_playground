cmake_minimum_required(VERSION 3.22)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 17)
 
# imgui
set(IMPLOT_INCLUDE extern/implot)
set(IMGUI_INCLUDE extern/imgui)
set(IMGUI_SRC 
    extern/imgui/imgui.cpp
    extern/imgui/imgui_draw.cpp
    extern/imgui/imgui_demo.cpp
    extern/imgui/imgui_widgets.cpp
    extern/imgui/imgui_tables.cpp
    extern/imgui/backends/imgui_impl_glfw.cpp
    extern/imgui/backends/imgui_impl_vulkan.cpp
    extern/implot/implot.cpp
    extern/implot/implot_items.cpp
)

# stb
set(STB_INCLUDE extern/stb)
set(STB_IMG_SRC extern/stb/stb_image_impl.cpp)

# spdlog
add_subdirectory(extern/spdlog)
set(SPDLOG_INCLUDE extern/spdlog/include)

# assimp
add_subdirectory(extern/assimp)
set(ASSIMP_INCLUDE extern/assimp/include)

# glfw
add_subdirectory(extern/glfw)
set(GLFW_INCLUDE extern/glfw/include)

# glm
set(GLM_INCLUDE extern/glm)

# tinyobjloader
set(TINY_OBJ_LOADER_INCLUDE extern/tinyobjloader)


project(vulkan_playground)

add_executable(vulkan_playground 
        src/main.cpp
        src/components/Logging.cpp
        src/components/ShaderUtils.cpp
        src/components/Animation.cpp
        src/components/DeltaTimer.cpp
        src/components/Camera.cpp
        src/structs/Vertex.cpp
        src/components/VulkanUtils.cpp
        src/components/ImGuiManager.cpp
        src/components/TextureManager.cpp
        src/components/InputManager.cpp
        src/lib/VQDevice.cpp
        src/lib/VQUtils.cpp
        src/VulkanEngine.cpp
        # render systems
        src/ecs/system/PhongRenderSystem.cpp
        src/ecs/system/PhongRenderSystemInstanced.cpp
        src/ecs/system/BindlessRenderSystem.cpp
        src/ecs/system/EntityViewerSystem.cpp
        src/ecs/system/GlobalGridSystem.cpp
        src/ecs/component/PhongRenderSystemInstancedComponent.cpp
        src/ecs/component/BindlessRenderSystemComponent.cpp
)

target_sources(${PROJECT_NAME} 
    PRIVATE ${IMGUI_SRC}
    PRIVATE ${STB_IMG_SRC}
)

target_include_directories(vulkan_playground
    PRIVATE ${STB_INCLUDE}
    PRIVATE ${IMGUI_INCLUDE}
    PRIVATE ${IMPLOT_INCLUDE}
    PRIVATE ${SPDLOG_INCLUDE}
    PRIVATE ${GLFW_INCLUDE}
    PRIVATE ${GLM_INCLUDE}
    PRIVATE ${TINY_OBJ_LOADER_INCLUDE}
    PRIVATE ${ASSIMP_INCLUDE}
    PRIVATE src
)


target_precompile_headers(vulkan_playground PUBLIC src/PCH.h)

# vulkan
if(APPLE)
    set(CMAKE_INSTALL_RPATH_USE_LINK_PATH ON)
    set(VULKAN_LIB_PATH $ENV{HOME}/lib/VulkanSDK/1.3.290.1/macOS/lib/libvulkan.1.3.290.dylib)
    set(MOLTENVK_LIB_PATH $ENV{HOME}/lib/VulkanSDK/1.3.290.1/macOS/lib/libMoltenVK.dylib)
    include_directories(~/lib/VulkanSDK/1.3.290.1/macOS/include)
    target_link_libraries(vulkan_playground 
        ${VULKAN_LIB_PATH}
        ${MOLTENVK_LIB_PATH}
    )
    target_compile_definitions(vulkan_playground PRIVATE VK_USE_PLATFORM_MACOS_MVK)
else() # ~APPLE
    find_package(Vulkan REQUIRED)
    target_link_libraries(vulkan_playground Vulkan::Vulkan)
endif() # APPLE

target_link_libraries(vulkan_playground 
    spdlog::spdlog 
    glfw
    assimp
)

# debug flag for unix
if(CMAKE_BUILD_TYPE MATCHES Release)
    add_compile_definitions(NDEBUG)
endif()
