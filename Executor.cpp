#include "Executor.h"

#include <fcntl.h>
#include <io.h>
#include <process.h>
#include <windows.h>

#include <cstring>
#include <iostream>

#include "Utils.h"

Executor::Executor() {}

Executor::~Executor() {}

int Executor::execute(const Command& command) { return executeCommand(command); }

int Executor::executeCommand(const Command& cmd) {
    std::string executable = findCommand(cmd.name);
    if (executable.empty()) {
        std::cerr << "Command not found: " << cmd.name << std::endl;
        return 127;
    }
    if (cmd.name == "cd") {
        if (cmd.arguments.empty()) {
            std::cout << "Please enter the name of the directory you want to change to."
                      << std::endl;
        } else if (cmd.arguments.size() > 1) {
            std::cout << "Please enter only one directory name." << std::endl;
        } else {
            if (SetCurrentDirectoryA(cmd.arguments[0].c_str())) {
                std::cout << "Changed directory to: " << cmd.arguments[0] << std::endl;
            } else {
                std::cerr << "Failed to change directory to: " << cmd.arguments[0] << std::endl;
                return 1;
            }
        }
    }
    std::string cmdLine = "\"" + executable + "\"";
    for (const auto& arg : cmd.arguments) {
        cmdLine += " \"" + arg + "\"";
    }

    STARTUPINFOA si;  // Use ANSI version
    PROCESS_INFORMATION pi;
    ZeroMemory(&si, sizeof(si));
    ZeroMemory(&pi, sizeof(pi));
    si.cb = sizeof(si);

    // Create the process
    BOOL success = CreateProcessA(NULL, const_cast<char*>(cmdLine.c_str()), NULL, NULL, TRUE, 0,
                                  NULL, NULL, &si, &pi);

    if (!success) {
        std::cerr << "Failed to create process: " << cmd.name << std::endl;
        return 1;
    }

    // Wait for the process to complete
    WaitForSingleObject(pi.hProcess, INFINITE);
    DWORD exitCode;
    GetExitCodeProcess(pi.hProcess, &exitCode);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    return static_cast<int>(exitCode);
}

char** Executor::createArgv(const Command& cmd) {
    char** argv = new char*[cmd.arguments.size() + 2];

    argv[0] = new char[cmd.name.length() + 1];
    strcpy(argv[0], cmd.name.c_str());

    for (size_t i = 0; i < cmd.arguments.size(); ++i) {
        argv[i + 1] = new char[cmd.arguments[i].length() + 1];
        strcpy(argv[i + 1], cmd.arguments[i].c_str());
    }

    argv[cmd.arguments.size() + 1] = nullptr;
    return argv;
}

void Executor::freeArgv(char** argv) {
    if (argv) {
        for (int i = 0; argv[i] != nullptr; ++i) {
            delete[] argv[i];
        }
        delete[] argv;
    }
}

std::string Executor::findCommand(const std::string& command) {
    std::string cmd = command;
    if (cmd.find('.') == std::string::npos) {
        std::vector<std::string> extensions = {".exe", ".com", ".bat", ".cmd"};
        for (const auto& ext : extensions) {
            std::string cmdWithExt = cmd + ext;
            if (Utils::fileExists(cmdWithExt)) {
                return cmdWithExt;
            }
        }
    }

    if (command.find('\\') != std::string::npos || command.find('/') != std::string::npos) {
        if (Utils::fileExists(command)) {
            return command;
        }
        return "";
    }

    // Search in PATH
    const char* pathEnv = getenv("PATH");
    if (!pathEnv) {
        return "";
    }

    std::vector<std::string> paths = Utils::split(std::string(pathEnv), ';');

    for (const std::string& path : paths) {
        // Try without extension first
        std::string fullPath = path + "\\" + command;
        if (Utils::fileExists(fullPath)) {
            return fullPath;
        }

        std::vector<std::string> extensions = {".exe", ".com", ".bat", ".cmd"};
        for (const auto& ext : extensions) {
            std::string fullPathWithExt = fullPath + ext;
            if (Utils::fileExists(fullPathWithExt)) {
                return fullPathWithExt;
            }
        }
    }

    return "";
}