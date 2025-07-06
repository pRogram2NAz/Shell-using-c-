#ifndef COMMAND_H
#define COMMAND_H

#include <string>
#include <vector>

struct Command {
    std::string name;
    std::vector<std::string> arguments;
    std::string inputRedirection;
    std::string outputRedirection;
    bool appendOutput;
    bool isBackground;

    Command() : appendOutput(false), isBackground(false) {}

    void clear();
    void print() const;
};

struct Pipeline {
    std::vector<Command> commands;

    void clear();
    bool empty() const;
    size_t size() const;
    Command& operator[](size_t index);
    const Command& operator[](size_t index) const;
};

#endif