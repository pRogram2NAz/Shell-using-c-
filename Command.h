#ifndef COMMAND_H
#define COMMAND_H

#include <string>
#include <vector>

struct Command {
    std::string name;
    std::vector<std::string> arguments;

    Command() {}

    void clear();
    void print() const;
};

#endif