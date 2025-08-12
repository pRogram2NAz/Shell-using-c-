#include "Shell.h"

#include <windows.h>

#include <algorithm>
#include <iostream>

#include "Utils.h"

Shell* g_shell = nullptr;

// Utility function to print colored text
void printColored(const std::string& text, WORD color) {
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(hConsole, &csbi);
    WORD originalColor = csbi.wAttributes;

    SetConsoleTextAttribute(hConsole, color);
    std::cout << text;
    SetConsoleTextAttribute(hConsole, originalColor);
}

BOOL WINAPI consoleHandler(DWORD dwCtrlType) {
    if (dwCtrlType == CTRL_C_EVENT) {
        // Print ^C in bright red
        printColored("^C", FOREGROUND_RED | FOREGROUND_INTENSITY);

        std::cout << "\n";
        if (g_shell) g_shell->displayPrompt();

        return TRUE;
    }
    return FALSE;
}

Shell::Shell() : builtins(this), running(true) {
    g_shell = this;
    currentDirectory = Utils::getCurrentWorkingDirectory();
    SetConsoleCtrlHandler(consoleHandler, TRUE);

    setEnvironmentVariable("PS1", "myshell> ");
    setEnvironmentVariable("HOME", Utils::getHomeDirectory());
    setEnvironmentVariable("PWD", currentDirectory);
    setEnvironmentVariable("SHELL", "myshell");
    setEnvironmentVariable("USER", getenv("USERNAME") ? getenv("USERNAME") : "unknown");
    setEnvironmentVariable("PATH", getenv("PATH") ? getenv("PATH") : "");
}

Shell::~Shell() { g_shell = nullptr; }

void Shell::run() {
    std::string input;

    std::cout << "Welcome to MyShell v1.0 (Windows)\n"
              << "Type 'help' for available commands or 'exit' to quit.\n\n";

    while (running) {
        displayPrompt();

        if (!std::getline(std::cin, input)) {
            std::cout << "\nEOF detected. Exiting shell.\nGoodbye!\n";
            break;
        }

        if (parser.isEmpty(input)) continue;

        try {
            Command command = parser.parse(input);

            if (builtins.isBuiltin(command.name)) {
                int result = builtins.execute(command.name, command.arguments);
                if (result == -1) running = false;
            } else {
                std::cerr << "'" << command.name << "': command not found" << std::endl;
                std::cerr << "Type 'help' for available commands." << std::endl;
            }
        } catch (const std::exception& e) {
            std::cerr << "Error: " << e.what() << std::endl;
        }
    }
}

void Shell::displayPrompt() {
    std::string prompt = getEnvironmentVariable("PS1");
    if (prompt.empty()) prompt = "myshell> ";

    size_t pos = prompt.find("\\w");
    if (pos != std::string::npos) {
        std::string shortPath = currentDirectory;
        std::string home = getEnvironmentVariable("HOME");
        if (!home.empty() && shortPath.find(home) == 0) {
            shortPath.replace(0, home.length(), "~");
        }
        prompt.replace(pos, 2, shortPath);
    }

    std::cout << prompt;
}

std::string Shell::getCurrentDirectory() const { return currentDirectory; }

void Shell::setCurrentDirectory(const std::string& dir) {
    currentDirectory = dir;
    setEnvironmentVariable("PWD", dir);
}

void Shell::setEnvironmentVariable(const std::string& name, const std::string& value) {
    environment[name] = value;
    _putenv_s(name.c_str(), value.c_str());
}

std::string Shell::getEnvironmentVariable(const std::string& name) const {
    auto it = environment.find(name);
    if (it != environment.end()) return it->second;

    const char* envValue = getenv(name.c_str());
    return envValue ? std::string(envValue) : std::string();
}

void Shell::displayEnvironmentVariables() const {
    std::vector<std::pair<std::string, std::string>> envVars;
    for (const auto& pair : environment) {
        envVars.push_back(pair);
    }

    std::sort(envVars.begin(), envVars.end());

    for (const auto& pair : envVars) {
        std::cout << pair.first << "=" << pair.second << std::endl;
    }
}

void Shell::shutdown() { running = false; }