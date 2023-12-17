#include "application/TriangleApp.h"

int main(int argc, char** argv) {
        INIT_LOGS();
        INFO("Logger initialized.");

        TriangleApp app;
        app.Run();


        return 0;
}