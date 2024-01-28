#include "application/DEngine.h"

int main(int argc, char** argv) {
    INIT_LOGS();
    INFO("Logger initialized.");

    DEngine engine;
    engine.Init();
    engine.Run();

    return 0;
}

