#include "BuiltinCommands.h"

#include <direct.h>
#include <windows.h>

#include <algorithm>
#include <cstring>
#include <iomanip>
#include <iostream>

#include "Shell.h"
#include "Utils.h"

BuiltinCommands::BuiltinCommands(Shell* shellPtr) : shell(shellPtr) {
    commands["cd"] = [this](const std::vector<std::string>& args) { return cmdCd(args); };
    commands["pwd"] = [this](const std::vector<std::string>& args) { return cmdPwd(args); };
    commands["echo"] = [this](const std::vector<std::string>& args) { return cmdEcho(args); };
    commands["help"] = [this](const std::vector<std::string>& args) { return cmdHelp(args); };
    commands["exit"] = [this](const std::vector<std::string>& args) { return cmdExit(args); };
    commands["env"] = [this](const std::vector<std::string>& args) { return cmdEnv(args); };
    commands["dir"] = [this](const std::vector<std::string>& args) { return cmdDir(args); };
}

BuiltinCommands::~BuiltinCommands() {}

bool BuiltinCommands::isBuiltin(const std::string& command) const {
    return commands.find(command) != commands.end();
}

int BuiltinCommands::execute(const std::string& command, const std::vector<std::string>& args) {
    auto it = commands.find(command);
    if (it != commands.end()) {
        return it->second(args);
    }
    return 1;
}

int BuiltinCommands::cmdCd(const std::vector<std::string>& args) {
    std::string targetDir;

    if (args.empty()) {
        targetDir = Utils::getHomeDirectory();
    } else if (args[0] == "-") {
        targetDir = shell->getEnvironmentVariable("OLDPWD");
        if (targetDir.empty()) {
            targetDir = Utils::getHomeDirectory();
        }
    } else {
        targetDir = args[0];
    }

    targetDir = Utils::expandTilde(targetDir);

    if (!targetDir.empty() && targetDir[0] != '\\' && targetDir[0] != '/' &&
        !(targetDir.length() > 1 && targetDir[1] == ':')) {
        targetDir = shell->getCurrentDirectory() + "\\" + targetDir;
    }

    targetDir = Utils::normalizePath(targetDir);

    if (!Utils::isDirectory(targetDir)) {
        std::cerr << "cd: " + targetDir + ": No such file or directory" << std::endl;
        return 1;
    }

    shell->setEnvironmentVariable("OLDPWD", shell->getCurrentDirectory());

    if (_chdir(targetDir.c_str()) != 0) {
        std::cerr << "cd: " + targetDir + ": Permission denied" << std::endl;
        return 1;
    }

    shell->setCurrentDirectory(targetDir);
    return 0;
}

int BuiltinCommands::cmdPwd(const std::vector<std::string>&) {
    std::string currentDir = shell->getCurrentDirectory();
    if (currentDir.empty()) {
        std::cerr << "pwd: error determining current directory" << std::endl;
        return 1;
    }
    std::cout << currentDir << std::endl;
    return 0;
}

int BuiltinCommands::cmdEcho(const std::vector<std::string>& args) {
    for (size_t i = 0; i < args.size(); ++i) {
        if (i > 0) std::cout << " ";
        std::cout << args[i];
    }
    std::cout << std::endl;
    return 0;
}

int BuiltinCommands::cmdExit(const std::vector<std::string>& args) {
    int exit_code = 0;
    if (!args.empty()) {
        try {
            exit_code = std::stoi(args[0]);
        } catch (const std::exception&) {
            std::cerr << "exit: " + args[0] + ": numeric argument required" << std::endl;
            return 1;
        }
    }
    std::cout << "Goodbye!" << std::endl;
    shell->shutdown();
    return -1;
}

int BuiltinCommands::cmdEnv(const std::vector<std::string>& args) {
    if (!args.empty()) {
        std::cerr << "env: too many arguments" << std::endl;
        return 1;
    }
    shell->displayEnvironmentVariables();
    return 0;
}

int BuiltinCommands::cmdDir(const std::vector<std::string>& args) {
    std::string path = args.empty() ? shell->getCurrentDirectory() : args[0];
    path = Utils::normalizePath(path);

    if (path.back() != '\\') path += "\\";
    path += "*";

    WIN32_FIND_DATAA findFileData;
    HANDLE hFind = FindFirstFileA(path.c_str(), &findFileData);

    if (hFind == INVALID_HANDLE_VALUE) {
        std::cerr << "dir: cannot access '"
                  << (args.empty() ? shell->getCurrentDirectory() : args[0]) << "'" << std::endl;
        return 1;
    }

    std::cout << "Directory of " << (args.empty() ? shell->getCurrentDirectory() : args[0])
              << "\n\n";

    do {
        if (strcmp(findFileData.cFileName, ".") == 0) continue;

        FILETIME localFileTime;
        SYSTEMTIME systemTime;
        FileTimeToLocalFileTime(&findFileData.ftLastWriteTime, &localFileTime);
        FileTimeToSystemTime(&localFileTime, &systemTime);

        std::cout << std::setfill('0') << std::setw(2) << systemTime.wMonth << "/" << std::setw(2)
                  << systemTime.wDay << "/" << systemTime.wYear << "  " << std::setw(2)
                  << systemTime.wHour << ":" << std::setw(2) << systemTime.wMinute << " ";

        if (findFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            std::cout << "    <DIR>          ";
        } else {
            LARGE_INTEGER fileSize;
            fileSize.LowPart = findFileData.nFileSizeLow;
            fileSize.HighPart = findFileData.nFileSizeHigh;
            std::cout << std::setw(15) << fileSize.QuadPart << " ";
        }

        std::cout << findFileData.cFileName << std::endl;
    } while (FindNextFileA(hFind, &findFileData) != 0);

    FindClose(hFind);
    return 0;
}

int BuiltinCommands::cmdHelp(const std::vector<std::string>& args) {
    if (args.empty()) {
        std::cout << "MyShell - Built-in Commands Help (Windows)\n"
                  << "==========================================\n\n"
                  << "Available commands:\n"
                  << "  cd [directory]     - Change current directory\n"
                  << "  pwd                - Print working directory\n"
                  << "  echo [text...]     - Display text\n"
                  << "  env                - Display environment variables\n"
                  << "  dir [path]         - List directory contents\n"
                  << "  exit [code]        - Exit the shell\n"
                  << "  help [command]     - Show help information\n\n"
                  << "Use 'help <command>' for detailed information about a specific command.\n";
    } else {
        const std::string& cmd = args[0];
        if (cmd == "cd") {
            std::cout << "cd - Change Directory\n"
                      << "Usage: cd [directory]\n"
                      << "  cd           - Go to home directory\n"
                      << "  cd -         - Go to previous directory\n"
                      << "  cd <dir>     - Go to specified directory\n";
        } else if (cmd == "pwd") {
            std::cout << "pwd - Print Working Directory\n"
                      << "Usage: pwd\n"
                      << "  Displays the current working directory\n";
        } else if (cmd == "echo") {
            std::cout << "echo - Display Text\n"
                      << "Usage: echo [text...]\n"
                      << "  Displays the given text followed by a newline\n";
        } else if (cmd == "env") {
            std::cout << "env - Display Environment Variables\n"
                      << "Usage: env\n"
                      << "  Displays all environment variables\n";
        } else if (cmd == "dir") {
            std::cout << "dir - List Directory Contents\n"
                      << "Usage: dir [path]\n"
                      << "  Lists files and directories in the specified path\n"
                      << "  If no path is specified, lists current directory\n";
        } else if (cmd == "exit") {
            std::cout << "exit - Exit Shell\n"
                      << "Usage: exit [code]\n"
                      << "  code  Optional exit code (default: 0)\n";
        } else if (isBuiltin(cmd)) {
            std::cout << "No detailed help available for '" << cmd << "'\n";
        } else {
            std::cerr << "help: " + cmd + ": no such builtin command" << std::endl;
            return 1;
        }
        std::cout << "\nFor general help, use 'help' without arguments.\n";
    }
    return 0;
}