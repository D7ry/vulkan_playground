#include <iostream>
#include "application/VulkanApplication.h"

int main(int argc, char **argv) {
        std::cout << "Hello, world!" << std::endl;
        VulkanApplication app;
        app.Run();
        return 0;
}