#include "application/VulkanApplication.h"

int main(int argc, char** argv) {
        Logging::Init();
        INFO("Logger initialized.");

        VulkanApplication app;
        app.Run();

        return 0;
}