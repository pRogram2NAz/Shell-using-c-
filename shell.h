#ifndef SHELL_H
#define SHELL_H

#include <string>
#include <unordered_map>
#include <vector>

#include "BuiltinCommands.h"
#include "Command.h"
#include "Executor.h"
#include "Parser.h"

class Shell {
   private:
    Parser parser;
    Executor executor;
    BuiltinCommands builtins;
    std::string currentDirectory;
    std::unordered_map<std::string, std::string> environment;
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
};

#endif