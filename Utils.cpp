#include "Utils.h"

#include <io.h>
#include <shlobj.h>
#include <windows.h>

#include <algorithm>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace Utils {

std::string trim(const std::string& str) {
    size_t start = str.find_first_not_of(" \t\n\r");
    if (start == std::string::npos) return "";

    size_t end = str.find_last_not_of(" \t\n\r");
    return str.substr(start, end - start + 1);
}

std::vector<std::string> split(const std::string& str, char delimiter) {
    std::vector<std::string> tokens;
    std::stringstream ss(str);
    std::string token;

    while (std::getline(ss, token, delimiter)) {
        if (!token.empty()) {
            tokens.push_back(token);
        }
    }

    return tokens;
}

bool endsWith(const std::string& str, const std::string& suffix) {
    return str.length() >= suffix.length() &&
           str.compare(str.length() - suffix.length(), suffix.length(), suffix) == 0;
}

bool fileExists(const std::string& path) { return _access(path.c_str(), 0) == 0; }

bool isDirectory(const std::string& path) {
    DWORD attributes = GetFileAttributesA(path.c_str());
    return (attributes != INVALID_FILE_ATTRIBUTES) && (attributes & FILE_ATTRIBUTE_DIRECTORY);
}

std::string expandTilde(const std::string& path) {
    if (path.empty() || path[0] != '~') {
        return path;
    }

    std::string home = getHomeDirectory();
    if (path.length() == 1) {
        return home;
    }

    if (path[1] == '\\' || path[1] == '/') {
        return home + path.substr(1);
    }

    return path;
}

std::string getHomeDirectory() {
    char path[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPathA(NULL, CSIDL_PROFILE, NULL, 0, path))) {
        return std::string(path);
    }

    const char* userProfile = getenv("USERPROFILE");
    if (userProfile) {
        return std::string(userProfile);
    }

    const char* homeDrive = getenv("HOMEDRIVE");
    const char* homePath = getenv("HOMEPATH");
    if (homeDrive && homePath) {
        return std::string(homeDrive) + std::string(homePath);
    }

    return "C:\\";
}

std::string getCurrentWorkingDirectory() {
    char buffer[MAX_PATH];
    DWORD result = GetCurrentDirectoryA(MAX_PATH, buffer);

    if (result == 0) {
        // Error occurred, return empty string
        return "";
    }

    if (result > MAX_PATH) {
        // Path is too long, allocate larger buffer
        std::vector<char> largeBuffer(result);
        DWORD result2 = GetCurrentDirectoryA(result, largeBuffer.data());
        if (result2 == 0) {
            return "";
        }
        return std::string(largeBuffer.data());
    }

    return std::string(buffer);
}

std::string normalizePath(const std::string& path) {
    std::string normalized = path;
    // Convert forward slashes to backslashes on Windows
    std::replace(normalized.begin(), normalized.end(), '/', '\\');
    return normalized;
}

// Color constants namespace
namespace Colors {
const std::string RESET = "\033[0m";
const std::string DIM = "\033[2m";
const std::string RED = "\033[31m";
const std::string GREEN = "\033[32m";
const std::string YELLOW = "\033[33m";
const std::string CYAN = "\033[36m";
}  // namespace Colors

// Utility functions for printing
void printError(const std::string& message) {
    std::cerr << Colors::RED << "Error: " << message << Colors::RESET << std::endl;
}

void printWarning(const std::string& message) {
    std::cout << Colors::YELLOW << "Warning: " << message << Colors::RESET << std::endl;
}

void printSuccess(const std::string& message) {
    std::cout << Colors::GREEN << "Success: " << message << Colors::RESET << std::endl;
}

void printInfo(const std::string& message) {
    std::cout << Colors::CYAN << "Info: " << message << Colors::RESET << std::endl;
}

void printDebug(const std::string& message) {
    std::cout << Colors::DIM << "Debug: " << message << Colors::RESET << std::endl;
}

}  