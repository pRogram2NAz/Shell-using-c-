#include <iostream>

#include "Shell.h"

int main() {
    try {
        Shell shell;
        shell.run();
    } catch (const std::exception& e) {
        std::cout << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}