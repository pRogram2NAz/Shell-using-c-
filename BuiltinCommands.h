#ifndef BUILTINCOMMANDS_H
#define BUILTINCOMMANDS_H

#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

class Shell;

class BuiltinCommands {
   private:
    Shell* shell;
    std::unordered_map<std::string, std::function<int(const std::vector<std::string>&)>> commands;

    int cmdCd(const std::vector<std::string>& args);
    int cmdPwd(const std::vector<std::string>& args);
    int cmdEcho(const std::vector<std::string>& args);
    int cmdHelp(const std::vector<std::string>& args);
    int cmdExit(const std::vector<std::string>& args);
    int cmdEnv(const std::vector<std::string>& args);

   public:
    BuiltinCommands(Shell* shellPtr);
    ~BuiltinCommands();

    bool isBuiltin(const std::string& command) const;
    int execute(const std::string& command, const std::vector<std::string>& args);
    void listCommands() const;
};

#endif