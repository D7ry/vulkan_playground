#include "application/VulkanApplication.h"
#include "spdlog/spdlog.h"

int main(int argc, char** argv) {
        INIT_LOGS();
        INFO("Logger initialized.");

        VulkanApplication app;
        app.Run();

        return 0;
}