#include "Executor.h"

#include <cstring>
#include <iostream>

#include "Utils.h"
#ifdef _WIN32
#include <fcntl.h>
#include <io.h>
#include <process.h>
#include <windows.h>
#else
#include <fcntl.h>
#include <signal.h>
#include <sys/wait.h>
#include <unistd.h>
#endif

Executor::Executor() {}

Executor::~Executor() { waitForBackgroundProcesses(); }

// Public execute method - this was missing!
int Executor::execute(const Pipeline& pipeline) {
    if (pipeline.empty()) {
        return 0;
    }

    if (pipeline.size() == 1) {
        return executeCommand(pipeline[0]);
    } else {
        return executePipeline(pipeline);
    }
}

#ifdef _WIN32
int Executor::executeCommand(const Command& cmd) {
    // Find the executable
    std::string executable = findCommand(cmd.name);
    if (executable.empty()) {
        std::cerr << "Command not found: " << cmd.name << std::endl;
        return 127;
    }

    // Prepare command line
    std::string cmdLine = "\"" + executable + "\"";
    for (const auto& arg : cmd.arguments) {
        cmdLine += " \"" + arg + "\"";
    }

    STARTUPINFOA si;  // Use ANSI version
    PROCESS_INFORMATION pi;
    ZeroMemory(&si, sizeof(si));
    ZeroMemory(&pi, sizeof(pi));
    si.cb = sizeof(si);

    // Handle redirections
    HANDLE hInputFile = INVALID_HANDLE_VALUE;
    HANDLE hOutputFile = INVALID_HANDLE_VALUE;

    if (!cmd.inputRedirection.empty()) {
        hInputFile = CreateFileA(cmd.inputRedirection.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL,
                                 OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
        if (hInputFile == INVALID_HANDLE_VALUE) {
            std::cerr << "Cannot open input file: " << cmd.inputRedirection << std::endl;
            return 1;
        }
        si.hStdInput = hInputFile;
        si.dwFlags |= STARTF_USESTDHANDLES;
    }

    if (!cmd.outputRedirection.empty()) {
        DWORD creationDisposition = cmd.appendOutput ? OPEN_ALWAYS : CREATE_ALWAYS;
        hOutputFile = CreateFileA(cmd.outputRedirection.c_str(), GENERIC_WRITE, FILE_SHARE_READ,
                                  NULL, creationDisposition, FILE_ATTRIBUTE_NORMAL, NULL);
        if (hOutputFile == INVALID_HANDLE_VALUE) {
            std::cerr << "Cannot open output file: " << cmd.outputRedirection << std::endl;
            if (hInputFile != INVALID_HANDLE_VALUE) CloseHandle(hInputFile);
            return 1;
        }
        if (cmd.appendOutput) {
            SetFilePointer(hOutputFile, 0, NULL, FILE_END);
        }
        si.hStdOutput = hOutputFile;
        si.dwFlags |= STARTF_USESTDHANDLES;
    }

    // Create the process
    BOOL success = CreateProcessA(NULL, const_cast<char*>(cmdLine.c_str()), NULL, NULL, TRUE, 0,
                                  NULL, NULL, &si, &pi);

    if (!success) {
        std::cerr << "Failed to create process: " << cmd.name << std::endl;
        if (hInputFile != INVALID_HANDLE_VALUE) CloseHandle(hInputFile);
        if (hOutputFile != INVALID_HANDLE_VALUE) CloseHandle(hOutputFile);
        return 1;
    }

    // Clean up file handles
    if (hInputFile != INVALID_HANDLE_VALUE) CloseHandle(hInputFile);
    if (hOutputFile != INVALID_HANDLE_VALUE) CloseHandle(hOutputFile);

    if (cmd.isBackground) {
        backgroundProcesses.push_back(pi.hProcess);
        std::cout << "[" << backgroundProcesses.size() << "] " << pi.dwProcessId << std::endl;
        CloseHandle(pi.hThread);
        return 0;
    } else {
        WaitForSingleObject(pi.hProcess, INFINITE);
        DWORD exitCode;
        GetExitCodeProcess(pi.hProcess, &exitCode);
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
        return static_cast<int>(exitCode);
    }
}

int Executor::executePipeline(const Pipeline& pipeline) {
    std::cout << "Pipeline execution on Windows (simplified mode)" << std::endl;

    int lastResult = 0;
    for (size_t i = 0; i < pipeline.size(); ++i) {
        const Command& cmd = pipeline[i];
        lastResult = executeCommand(cmd);
        if (lastResult != 0) {
            std::cerr << "Pipeline stopped due to error in command: " << cmd.name << std::endl;
            break;
        }
    }
    return lastResult;
}

#else
// POSIX implementation (Linux/Unix)
int Executor::executeCommand(const Command& cmd) {
    // Find the executable
    std::string executable = findCommand(cmd.name);
    if (executable.empty()) {
        std::cerr << "Command not found: " << cmd.name << std::endl;
        return 127;
    }

    pid_t pid = fork();

    if (pid == 0) {
        // Child process

        // Set up redirections
        if (!cmd.inputRedirection.empty()) {
            int fd = open(cmd.inputRedirection.c_str(), O_RDONLY);
            if (fd == -1) {
                perror(("Cannot open " + cmd.inputRedirection).c_str());
                exit(1);
            }
            dup2(fd, STDIN_FILENO);
            close(fd);
        }

        if (!cmd.outputRedirection.empty()) {
            int flags = O_WRONLY | O_CREAT;
            flags |= cmd.appendOutput ? O_APPEND : O_TRUNC;
            int fd = open(cmd.outputRedirection.c_str(), flags, 0644);
            if (fd == -1) {
                perror(("Cannot open " + cmd.outputRedirection).c_str());
                exit(1);
            }
            dup2(fd, STDOUT_FILENO);
            close(fd);
        }

        // Prepare arguments
        char** argv = createArgv(cmd);

        // Execute the command
        execv(executable.c_str(), argv);

        // If we get here, execv failed
        perror(("Cannot execute " + cmd.name).c_str());
        exit(127);
    } else if (pid > 0) {
        // Parent process
        if (cmd.isBackground) {
            backgroundProcesses.push_back(pid);
            std::cout << "[" << backgroundProcesses.size() << "] " << pid << std::endl;
            return 0;
        } else {
            int status;
            waitpid(pid, &status, 0);
            return WEXITSTATUS(status);
        }
    } else {
        perror("fork failed");
        return 1;
    }
}

int Executor::executePipeline(const Pipeline& pipeline) {
    std::vector<int> pipes;
    std::vector<pid_t> pids;

    // Create pipes
    for (size_t i = 0; i < pipeline.size() - 1; ++i) {
        int pipefd[2];
        if (pipe(pipefd) == -1) {
            perror("pipe failed");
            return 1;
        }
        pipes.push_back(pipefd[0]);  // read end
        pipes.push_back(pipefd[1]);  // write end
    }

    // Execute each command in the pipeline
    for (size_t i = 0; i < pipeline.size(); ++i) {
        const Command& cmd = pipeline[i];
        std::string executable = findCommand(cmd.name);

        if (executable.empty()) {
            std::cerr << "Command not found: " << cmd.name << std::endl;
            continue;
        }

        pid_t pid = fork();

        if (pid == 0) {
            // Child process

            // Set up input redirection
            if (i > 0) {
                // Not the first command, get input from previous pipe
                dup2(pipes[(i - 1) * 2], STDIN_FILENO);
            } else if (!cmd.inputRedirection.empty()) {
                // First command with input redirection
                int fd = open(cmd.inputRedirection.c_str(), O_RDONLY);
                if (fd == -1) {
                    perror(("Cannot open " + cmd.inputRedirection).c_str());
                    exit(1);
                }
                dup2(fd, STDIN_FILENO);
                close(fd);
            }

            // Set up output redirection
            if (i < pipeline.size() - 1) {
                // Not the last command, send output to next pipe
                dup2(pipes[i * 2 + 1], STDOUT_FILENO);
            } else if (!cmd.outputRedirection.empty()) {
                // Last command with output redirection
                int flags = O_WRONLY | O_CREAT;
                flags |= cmd.appendOutput ? O_APPEND : O_TRUNC;
                int fd = open(cmd.outputRedirection.c_str(), flags, 0644);
                if (fd == -1) {
                    perror(("Cannot open " + cmd.outputRedirection).c_str());
                    exit(1);
                }
                dup2(fd, STDOUT_FILENO);
                close(fd);
            }

            // Close all pipe file descriptors
            for (int fd : pipes) {
                close(fd);
            }

            // Prepare arguments and execute
            char** argv = createArgv(cmd);
            execv(executable.c_str(), argv);

            perror(("Cannot execute " + cmd.name).c_str());
            exit(127);
        } else if (pid > 0) {
            pids.push_back(pid);
        } else {
            perror("fork failed");
        }
    }

    // Close all pipe file descriptors in parent
    for (int fd : pipes) {
        close(fd);
    }

    // Wait for all processes
    int lastStatus = 0;
    for (pid_t pid : pids) {
        int status;
        waitpid(pid, &status, 0);
        lastStatus = WEXITSTATUS(status);
    }

    return lastStatus;
}
#endif

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
#ifdef _WIN32
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
#else

    if (command.find('/') != std::string::npos) {
        if (Utils::fileExists(command) && Utils::isExecutable(command)) {
            return command;
        }
        return "";
    }

    // Search in PATH
    const char* pathEnv = getenv("PATH");
    if (!pathEnv) {
        return "";
    }

    std::vector<std::string> paths = Utils::split(std::string(pathEnv), ':');

    for (const std::string& path : paths) {
        std::string fullPath = path + "/" + command;
        if (Utils::fileExists(fullPath) && Utils::isExecutable(fullPath)) {
            return fullPath;
        }
    }

    return "";
#endif
}

void Executor::waitForBackgroundProcesses() {
#ifdef _WIN32
    for (HANDLE hProcess : backgroundProcesses) {
        if (hProcess != INVALID_HANDLE_VALUE) {
            WaitForSingleObject(hProcess, INFINITE);
            CloseHandle(hProcess);
        }
    }
#else
    for (pid_t pid : backgroundProcesses) {
        int status;
        waitpid(pid, &status, 0);
    }
#endif
    backgroundProcesses.clear();
}

void Executor::cleanupZombies() {
#ifdef _WIN32

    for (auto it = backgroundProcesses.begin(); it != backgroundProcesses.end();) {
        DWORD result = WaitForSingleObject(*it, 0);
        if (result == WAIT_OBJECT_0) {
            CloseHandle(*it);
            it = backgroundProcesses.erase(it);
        } else {
            ++it;
        }
    }
#else
    for (auto it = backgroundProcesses.begin(); it != backgroundProcesses.end();) {
        int status;
        pid_t result = waitpid(*it, &status, WNOHANG);
        if (result > 0) {
            it = backgroundProcesses.erase(it);
        } else {
            ++it;
        }
    }
#endif
}

void Executor::setupRedirection(const Command& cmd) {}

void Executor::restoreRedirection(int savedStdin, int savedStdout) {}