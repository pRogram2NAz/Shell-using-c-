#include "Command.h"

#include <iostream>

void Command::clear() {
    name.clear();
    arguments.clear();
    inputRedirection.clear();
    outputRedirection.clear();
    appendOutput = false;
    isBackground = false;
}

void Command::print() const {
    std::cout << "Command: " << name << std::endl;
    std::cout << "Arguments: ";
    for (const auto& arg : arguments) {
        std::cout << "[" << arg << "] ";
    }
    std::cout << std::endl;

    if (!inputRedirection.empty()) {
        std::cout << "Input redirection: " << inputRedirection << std::endl;
    }

    if (!outputRedirection.empty()) {
        std::cout << "Output redirection: " << outputRedirection
                  << (appendOutput ? " (append)" : " (overwrite)") << std::endl;
    }

    if (isBackground) {
        std::cout << "Background process: yes" << std::endl;
    }

    std::cout << "---" << std::endl;
}

void Pipeline::clear() { commands.clear(); }

bool Pipeline::empty() const { return commands.empty(); }

size_t Pipeline::size() const { return commands.size(); }

Command& Pipeline::operator[](size_t index) { return commands[index]; }

const Command& Pipeline::operator[](size_t index) const { return commands[index]; }