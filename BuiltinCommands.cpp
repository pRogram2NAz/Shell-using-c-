#include "BuiltinCommands.h"

#include <direct.h>
#include <io.h>
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
    commands["cls"] = [this](const std::vector<std::string>& args) { return cmdCls(args); };
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

void BuiltinCommands::listCommands() const {
    std::cout << "Available builtin commands:\n";
    for (const auto& pair : commands) {
        std::cout << "  " << pair.first << std::endl;
    }
}

// Individual command implementations

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

    // Handle relative paths - Windows uses backslashes
    if (!targetDir.empty() && targetDir[0] != '\\' && targetDir[0] != '/' &&
        !(targetDir.length() > 1 && targetDir[1] == ':')) {
        targetDir = shell->getCurrentDirectory() + "\\" + targetDir;
    }

    // Normalize path
    targetDir = Utils::normalizePath(targetDir);

    // Check if directory exists
    if (!Utils::isDirectory(targetDir)) {
        Utils::printError("cd: " + targetDir + ": No such file or directory");
        return 1;
    }

    // Save current directory as OLDPWD
    shell->setEnvironmentVariable("OLDPWD", shell->getCurrentDirectory());

    // Change directory
    if (_chdir(targetDir.c_str()) != 0) {
        Utils::printError("cd: " + targetDir + ": Permission denied");
        return 1;
    }

    // Update shell's current directory
    shell->setCurrentDirectory(targetDir);

    return 0;
}

int BuiltinCommands::cmdPwd(const std::vector<std::string>& args) {
    // Suppress unused parameter warning
    (void)args;

    std::string currentDir = shell->getCurrentDirectory();

    if (currentDir.empty()) {
        Utils::printError("pwd: error determining current directory");
        return 1;
    }

    std::cout << currentDir << std::endl;
    return 0;
}

int BuiltinCommands::cmdEcho(const std::vector<std::string>& args) {
    for (size_t i = 0; i < args.size(); ++i) {
        if (i > 0) {
            std::cout << " ";
        }
        std::cout << args[i];
    }
    std::cout << std::endl;
    return 0;
}

int BuiltinCommands::cmdExit(const std::vector<std::string>& args) {
    int exit_code = 0;

    // Check if an exit code was provided
    if (!args.empty()) {
        try {
            exit_code = std::stoi(args[0]);
        } catch (const std::exception&) {
            Utils::printError("exit: " + args[0] + ": numeric argument required");
            return 1;
        }
    }

    std::cout << "Goodbye!" << std::endl;

    // Signal to the shell that it should exit
    shell->shutdown();

    // Return -1 to indicate exit request (special return code)
    return -1;
}

int BuiltinCommands::cmdEnv(const std::vector<std::string>& args) {
    if (!args.empty()) {
        Utils::printError("env: too many arguments");
        return 1;
    }

    shell->displayEnvironmentVariables();
    return 0;
}

int BuiltinCommands::cmdCls(const std::vector<std::string>& args) {
    // Suppress unused parameter warning
    (void)args;

    // Clear screen using Windows API
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    DWORD count;
    DWORD cellCount;
    COORD homeCoords = {0, 0};

    if (hConsole == INVALID_HANDLE_VALUE) {
        return 1;
    }

    // Get the number of cells in the current buffer
    if (!GetConsoleScreenBufferInfo(hConsole, &csbi)) {
        return 1;
    }
    cellCount = csbi.dwSize.X * csbi.dwSize.Y;

    // Fill the entire buffer with spaces
    if (!FillConsoleOutputCharacter(hConsole, (TCHAR)' ', cellCount, homeCoords, &count)) {
        return 1;
    }

    // Fill the entire buffer with the current colors and attributes
    if (!FillConsoleOutputAttribute(hConsole, csbi.wAttributes, cellCount, homeCoords, &count)) {
        return 1;
    }

    // Move the cursor home
    SetConsoleCursorPosition(hConsole, homeCoords);

    return 0;
}

int BuiltinCommands::cmdDir(const std::vector<std::string>& args) {
    std::string path = args.empty() ? shell->getCurrentDirectory() : args[0];

    // Normalize path
    path = Utils::normalizePath(path);

    // Add wildcard for directory listing
    if (path.back() != '\\') {
        path += "\\";
    }
    path += "*";

    WIN32_FIND_DATAA findFileData;
    HANDLE hFind = FindFirstFileA(path.c_str(), &findFileData);

    if (hFind == INVALID_HANDLE_VALUE) {
        Utils::printError("dir: cannot access '" +
                          (args.empty() ? shell->getCurrentDirectory() : args[0]) + "'");
        return 1;
    }

    std::cout << "Directory of " << (args.empty() ? shell->getCurrentDirectory() : args[0])
              << "\n\n";

    do {
        // Skip current directory entry
        if (strcmp(findFileData.cFileName, ".") == 0) {
            continue;
        }

        // Format file time
        FILETIME localFileTime;
        SYSTEMTIME systemTime;
        FileTimeToLocalFileTime(&findFileData.ftLastWriteTime, &localFileTime);
        FileTimeToSystemTime(&localFileTime, &systemTime);

        // Format date and time with proper padding
        std::cout << std::setfill('0') << std::setw(2) << systemTime.wMonth << "/" << std::setw(2)
                  << systemTime.wDay << "/" << systemTime.wYear << "  " << std::setw(2)
                  << systemTime.wHour << ":" << std::setw(2) << systemTime.wMinute << " ";

        // Check if it's a directory
        if (findFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            std::cout << "    <DIR>          ";
        } else {
            // Show file size
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
        std::cout << "MyShell - Built-in Commands Help (Windows)\n";
        std::cout << "==========================================\n\n";

        std::cout << "Available commands:\n";
        std::cout << "  cd [directory]     - Change current directory\n";
        std::cout << "  pwd                - Print working directory\n";
        std::cout << "  echo [text...]     - Display text\n";
        std::cout << "  env                - Display environment variables\n";
        std::cout << "  cls                - Clear screen\n";
        std::cout << "  dir [path]         - List directory contents\n";
        std::cout << "  exit [code]        - Exit the shell\n";
        std::cout << "  help [command]     - Show help information\n\n";

        std::cout << "Use 'help <command>' for detailed information about a specific command.\n";
    } else {
        // Help for specific command
        const std::string& cmd = args[0];

        if (cmd == "cd") {
            std::cout << "cd - Change Directory\n";
            std::cout << "Usage: cd [directory]\n";
            std::cout << "  cd           - Go to home directory\n";
            std::cout << "  cd -         - Go to previous directory\n";
            std::cout << "  cd <dir>     - Go to specified directory\n";
            std::cout << "  Examples:\n";
            std::cout << "    cd C:\\Users\\Username\n";
            std::cout << "    cd ..\n";
            std::cout << "    cd Documents\n";
        } else if (cmd == "pwd") {
            std::cout << "pwd - Print Working Directory\n";
            std::cout << "Usage: pwd\n";
            std::cout << "  Displays the current working directory\n";
        } else if (cmd == "echo") {
            std::cout << "echo - Display Text\n";
            std::cout << "Usage: echo [text...]\n";
            std::cout << "  Displays the given text followed by a newline\n";
        } else if (cmd == "env") {
            std::cout << "env - Display Environment Variables\n";
            std::cout << "Usage: env\n";
            std::cout << "  Displays all environment variables\n";
        } else if (cmd == "cls") {
            std::cout << "cls - Clear Screen\n";
            std::cout << "Usage: cls\n";
            std::cout << "  Clears the console screen\n";
        } else if (cmd == "dir") {
            std::cout << "dir - List Directory Contents\n";
            std::cout << "Usage: dir [path]\n";
            std::cout << "  Lists files and directories in the specified path\n";
            std::cout << "  If no path is specified, lists current directory\n";
        } else if (cmd == "exit") {
            std::cout << "exit - Exit Shell\n";
            std::cout << "Usage: exit [code]\n";
            std::cout << "  code  Optional exit code (default: 0)\n";
        } else if (isBuiltin(cmd)) {
            std::cout << "No detailed help available for '" << cmd << "'\n";
        } else {
            Utils::printError("help: " + cmd + ": no such builtin command");
            return 1;
        }
        std::cout << "\nFor general help, use 'help' without arguments.\n";
    }

    return 0;
}