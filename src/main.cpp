#include "application/VulkanApplication.h"
#include <iostream>

int main(int argc, char** argv) {
        INFO("Initializing Vulkan Playground...");
        VulkanApplication app;
        app.Run();

        return 0;
}