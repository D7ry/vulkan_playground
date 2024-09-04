#include "VulkanEngine.h"

int main(int argc, char** argv) {
    INIT_LOGS();
    INFO("Logger initialized.");
    DEBUG("running in debug mode");
    VulkanEngine engine;
    engine.Init();
    engine.Run();
    engine.Cleanup();


    return 0;
}

