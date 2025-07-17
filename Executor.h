#ifndef EXECUTOR_H
#define EXECUTOR_H

#include <string>
#include <vector>
#ifdef _WIN32
#include <process.h>
#include <windows.h>
#endif
#include "Command.h"

class Executor {
   private:
    int executeCommand(const Command& cmd);
    char** createArgv(const Command& cmd);
    void freeArgv(char** argv);

   public:
    Executor();
    ~Executor();

    int execute(const Command& command);
    std::string findCommand(const std::string& command);
};

#endif