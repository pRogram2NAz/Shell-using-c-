#ifndef SHELL_H
#define SHELL_H

#include <csignal>
#include <string>
#include <unordered_map>

#include "BuiltinCommands.h"
#include "Executor.h"
#include "Parser.h"

class Shell {
   private:
    std::string currentDirectory;
    std::unordered_map<std::string, std::string> environment;
    BuiltinCommands builtins;
    Executor executor;
    Parser parser;
    bool running;

   public:
    Shell();
    ~Shell();

    void run();
    void displayPrompt();
    std::string getCurrentDirectory() const;
    void setCurrentDirectory(const std::string& dir);
    void setEnvironmentVariable(const std::string& name, const std::string& value);
    std::string getEnvironmentVariable(const std::string& name) const;
    void displayEnvironmentVariables() const;
    void shutdown();

#ifdef _WIN32
    friend BOOL WINAPI consoleHandler(DWORD dwCtrlType);
#else
    static void signalHandler(int signal);
#endif
};

#endif