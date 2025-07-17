#include "Command.h"

#include <iostream>

void Command::clear() {
    name.clear();
    arguments.clear();
}

void Command::print() const {
    std::cout << "Command: " << name << std::endl;
    std::cout << "Arguments: ";
    for (const auto& arg : arguments) {
        std::cout << "[" << arg << "] ";
    }
    std::cout << std::endl;
    std::cout << "---" << std::endl;
}