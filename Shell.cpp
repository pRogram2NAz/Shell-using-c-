#include "Shell.h"

#include <algorithm>
#include <iostream>

#include "Utils.h"

#ifdef _WIN32
#include <direct.h>
#include <io.h>
#include <windows.h>
#else
#include <signal.h>
#include <unistd.h>
#endif

Shell* g_shell = nullptr;

#ifdef _WIN32
BOOL WINAPI consoleHandler(DWORD dwCtrlType) {
    switch (dwCtrlType) {
        case CTRL_C_EVENT:
            std::cout << "\n";
            if (g_shell) {
                g_shell->displayPrompt();
            }
            return TRUE;
        default:
            return FALSE;
    }
}
#else
void signalHandler(int signum) {
    if (signum == SIGINT) {
        std::cout << "\n";
        if (g_shell) {
            g_shell->displayPrompt();
        }
    }
}
#endif

Shell::Shell() : builtins(this), running(true) {
    g_shell = this;
    currentDirectory = Utils::getCurrentWorkingDirectory();

    // Set up signal handling
#ifdef _WIN32
    SetConsoleCtrlHandler(consoleHandler, TRUE);
#else
    signal(SIGINT, signalHandler);
#endif

    // Initialize some basic environment variables
    setEnvironmentVariable("PS1", "myshell> ");
    setEnvironmentVariable("HOME", Utils::getHomeDirectory());
    setEnvironmentVariable("PWD", currentDirectory);
    setEnvironmentVariable("SHELL", "myshell");
    setEnvironmentVariable("USER", getenv("USER") ? getenv("USER") : "unknown");
    setEnvironmentVariable("PATH", getenv("PATH") ? getenv("PATH") : "");
}

Shell::~Shell() { g_shell = nullptr; }

void Shell::run() {
    std::string input;

    std::cout << "Welcome to MyShell v1.0\n";
    std::cout << "Type 'help' for available commands or 'exit' to quit.\n\n";

    while (running) {
        displayPrompt();

        if (!std::getline(std::cin, input)) {  // EOF handling
            std::cout << "\nEOF detected. Exiting shell.\n";
            std::cout << "\nGoodbye!\n";
            break;
        }

        if (parser.isEmpty(input)) {
            continue;
        }

        try {
            Pipeline pipeline = parser.parse(input);

            if (!pipeline.empty()) {
                // Check if it's a builtin command
                if (pipeline.size() == 1 && builtins.isBuiltin(pipeline[0].name)) {
                    int result = builtins.execute(pipeline[0].name, pipeline[0].arguments);
                    if (result == -1) {  // Special case for exit command
                        running = false;
                    }
                } else {
                    executor.execute(pipeline);
                }
            }
        } catch (const std::exception& e) {
            Utils::printError("Error: " + std::string(e.what()));
        }

        // Clean up any zombie processes
        executor.cleanupZombies();
    }
}

void Shell::displayPrompt() {
    std::string prompt = getEnvironmentVariable("PS1");
    if (prompt.empty()) {
        prompt = "myshell> ";
    }

    size_t pos = prompt.find("\\w");
    if (pos != std::string::npos) {
        std::string shortPath = currentDirectory;
        std::string home = getEnvironmentVariable("HOME");
        if (!home.empty() && shortPath.find(home) == 0) {
            shortPath.replace(0, home.length(), "~");
        }
        prompt.replace(pos, 2, shortPath);
    }

    std::cout << Utils::Colors::CYAN << prompt << Utils::Colors::RESET;
}

std::string Shell::getCurrentDirectory() const { return currentDirectory; }

void Shell::setCurrentDirectory(const std::string& dir) {
    currentDirectory = dir;
    setEnvironmentVariable("PWD", dir);
}

void Shell::setEnvironmentVariable(const std::string& name, const std::string& value) {
    environment[name] = value;
#ifdef _WIN32
    _putenv_s(name.c_str(), value.c_str());
#else
    setenv(name.c_str(), value.c_str(), 1);
#endif
}

std::string Shell::getEnvironmentVariable(const std::string& name) const {
    auto it = environment.find(name);
    if (it != environment.end()) {
        return it->second;
    }

    const char* envValue = getenv(name.c_str());
    return envValue ? std::string(envValue) : std::string();
}

void Shell::displayEnvironmentVariables() const {
    // Create a vector of pairs for sorting
    std::vector<std::pair<std::string, std::string>> envVars;

    // Add shell's custom environment variables
    for (const auto& pair : environment) {
        envVars.push_back(pair);
    }

    // Sort alphabetically by variable name
    std::sort(envVars.begin(), envVars.end());

    // Display sorted environment variables
    for (const auto& pair : envVars) {
        std::cout << pair.first << "=" << pair.second << std::endl;
    }
}

void Shell::shutdown() { running = false; }