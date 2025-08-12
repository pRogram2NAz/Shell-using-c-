#include "Executor.h"

#include <iostream>

Executor::Executor() {}

Executor::~Executor() {}

int Executor::execute(const Command& command) {
    std::cerr << "External command execution not supported: " << command.name << std::endl;
    return 127;
}

std::string Executor::findCommand(const std::string& command) { return ""; }