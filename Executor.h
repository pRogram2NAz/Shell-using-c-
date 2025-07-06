#ifndef EXECUTOR_H
#define EXECUTOR_H

#include <string>
#include <vector>
#ifdef _WIN32
#include <process.h>
#include <windows.h>
#else
#include <sys/types.h>
#endif
#include "Command.h"

class Executor {
   private:
#ifdef _WIN32
    std::vector<HANDLE> backgroundProcesses;
#else
    std::vector<pid_t> backgroundProcesses;
#endif

    int executeCommand(const Command& cmd);
    int executePipeline(const Pipeline& pipeline);
    void setupRedirection(const Command& cmd);
    void restoreRedirection(int savedStdin, int savedStdout);
    char** createArgv(const Command& cmd);
    void freeArgv(char** argv);

   public:
    Executor();
    ~Executor();

    int execute(const Pipeline& pipeline);
    void waitForBackgroundProcesses();
    void cleanupZombies();
    std::string findCommand(const std::string& command);
};

#endif