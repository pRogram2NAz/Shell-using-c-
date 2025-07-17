#include "Utils.h"

#include <algorithm>
#include <cerrno>
#include <cstring>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

// Windows-specific includes
#include <direct.h>
#include <io.h>
#include <shlobj.h>
#include <windows.h>

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

std::string join(const std::vector<std::string>& strings, const std::string& delimiter) {
    if (strings.empty()) return "";

    std::string result = strings[0];
    for (size_t i = 1; i < strings.size(); ++i) {
        result += delimiter + strings[i];
    }

    return result;
}

bool startsWith(const std::string& str, const std::string& prefix) {
    return str.length() >= prefix.length() && str.compare(0, prefix.length(), prefix) == 0;
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

bool isExecutable(const std::string& path) {
    // On Windows, check if file exists and has executable extension
    if (!fileExists(path)) return false;

    std::string lowerPath = path;
    std::transform(lowerPath.begin(), lowerPath.end(), lowerPath.begin(), ::tolower);

    return endsWith(lowerPath, ".exe") || endsWith(lowerPath, ".com") ||
           endsWith(lowerPath, ".bat") || endsWith(lowerPath, ".cmd");
}

std::string getAbsolutePath(const std::string& path) {
    char fullPath[MAX_PATH];
    if (_fullpath(fullPath, path.c_str(), MAX_PATH) != nullptr) {
        return std::string(fullPath);
    }
    return path;
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
    if (_getcwd(buffer, MAX_PATH) != nullptr) {
        return std::string(buffer);
    }
    return "";
}

bool createDirectory(const std::string& path) {
    return _mkdir(path.c_str()) == 0 || errno == EEXIST;
}

bool createDirectories(const std::string& path) {
    if (path.empty() || isDirectory(path)) {
        return true;
    }

    std::string parent = path;
    size_t pos = parent.find_last_of("/\\");
    if (pos != std::string::npos) {
        parent = parent.substr(0, pos);
        if (!createDirectories(parent)) {
            return false;
        }
    }

    return createDirectory(path);
}

std::string readFile(const std::string& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open()) {
        return "";
    }

    std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    return content;
}

bool writeFile(const std::string& path, const std::string& content) {
    std::ofstream file(path, std::ios::binary);
    if (!file.is_open()) {
        return false;
    }

    file << content;
    return file.good();
}

std::string getFileExtension(const std::string& path) {
    size_t dotPos = path.find_last_of('.');
    size_t slashPos = path.find_last_of("/\\");

    if (dotPos == std::string::npos || (slashPos != std::string::npos && dotPos < slashPos)) {
        return "";
    }

    return path.substr(dotPos + 1);
}

std::string getFileName(const std::string& path) {
    size_t pos = path.find_last_of("/\\");
    if (pos == std::string::npos) {
        return path;
    }
    return path.substr(pos + 1);
}

std::string getDirectoryName(const std::string& path) {
    size_t pos = path.find_last_of("/\\");
    if (pos == std::string::npos) {
        return ".";
    }
    if (pos == 0) {
        return "\\";
    }
    return path.substr(0, pos);
}

std::string toLowerCase(const std::string& str) {
    std::string result = str;
    std::transform(result.begin(), result.end(), result.begin(), ::tolower);
    return result;
}

std::string toUpperCase(const std::string& str) {
    std::string result = str;
    std::transform(result.begin(), result.end(), result.begin(), ::toupper);
    return result;
}

std::string replace(const std::string& str, const std::string& from, const std::string& to) {
    if (from.empty()) {
        return str;
    }

    std::string result = str;
    size_t pos = 0;
    while ((pos = result.find(from, pos)) != std::string::npos) {
        result.replace(pos, from.length(), to);
        pos += to.length();
    }
    return result;
}

bool isWhitespace(const std::string& str) {
    return str.find_first_not_of(" \t\n\r") == std::string::npos;
}

size_t getFileSize(const std::string& path) {
    HANDLE hFile = CreateFileA(path.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING,
                               FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) {
        return 0;
    }

    LARGE_INTEGER fileSize;
    if (GetFileSizeEx(hFile, &fileSize)) {
        CloseHandle(hFile);
        return static_cast<size_t>(fileSize.QuadPart);
    }

    CloseHandle(hFile);
    return 0;
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
const std::string BOLD = "\033[1m";
const std::string DIM = "\033[2m";
const std::string UNDERLINE = "\033[4m";
const std::string BLINK = "\033[5m";
const std::string REVERSE = "\033[7m";
const std::string HIDDEN = "\033[8m";

// Regular colors
const std::string BLACK = "\033[30m";
const std::string RED = "\033[31m";
const std::string GREEN = "\033[32m";
const std::string YELLOW = "\033[33m";
const std::string BLUE = "\033[34m";
const std::string MAGENTA = "\033[35m";
const std::string CYAN = "\033[36m";
const std::string WHITE = "\033[37m";

// Bright colors
const std::string BRIGHT_BLACK = "\033[90m";
const std::string BRIGHT_RED = "\033[91m";
const std::string BRIGHT_GREEN = "\033[92m";
const std::string BRIGHT_YELLOW = "\033[93m";
const std::string BRIGHT_BLUE = "\033[94m";
const std::string BRIGHT_MAGENTA = "\033[95m";
const std::string BRIGHT_CYAN = "\033[96m";
const std::string BRIGHT_WHITE = "\033[97m";

// Background colors
const std::string BG_BLACK = "\033[40m";
const std::string BG_RED = "\033[41m";
const std::string BG_GREEN = "\033[42m";
const std::string BG_YELLOW = "\033[43m";
const std::string BG_BLUE = "\033[44m";
const std::string BG_MAGENTA = "\033[45m";
const std::string BG_CYAN = "\033[46m";
const std::string BG_WHITE = "\033[47m";
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

}  // namespace Utils