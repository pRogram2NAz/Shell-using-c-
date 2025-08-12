#ifndef EXECUTOR_H
#define EXECUTOR_H

#include <string>

#include "Command.h"

class Executor {
   public:
    Executor();
    ~Executor();

    int execute(const Command& command);
    std::string findCommand(const std::string& command);
};

#endif