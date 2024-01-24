#include "application/DEngine.h"

int main(int argc, char** argv) {
    INIT_LOGS();
    INFO("Logger initialized.");

    DEngine engine;
    engine.Init();
    engine.Run();

    return 0;
}

void fib(int n, int& ret) {
    if (n == 0) {
        ret = 0;
        return;
    } else if (n == 1) {
        ret = 1;
        return;
    } else {
        int ret1 = 0;
        int ret2 = 0;
        fib(n - 1, ret1);
        fib(n - 2, ret2);
        ret = ret1 + ret2;
        return;
    }
}
