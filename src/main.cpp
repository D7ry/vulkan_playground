#include "application/VulkanApplication.h"
#include "application/TriangleApp.h"
#include "spdlog/spdlog.h"

int main(int argc, char** argv) {
        INIT_LOGS();
        INFO("Logger initialized.");

        TriangleApp app;
        app.Run();


        return 0;
}