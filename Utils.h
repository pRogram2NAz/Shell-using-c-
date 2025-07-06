#ifndef UTILS_H
#define UTILS_H

#include <string>
#include <vector>

namespace Utils {

std::string trim(const std::string& str);
std::vector<std::string> split(const std::string& str, char delimiter);
std::string join(const std::vector<std::string>& strings, const std::string& delimiter);
bool startsWith(const std::string& str, const std::string& prefix);
bool endsWith(const std::string& str, const std::string& suffix);
std::string toLowerCase(const std::string& str);
std::string toUpperCase(const std::string& str);
std::string replace(const std::string& str, const std::string& from, const std::string& to);
bool isWhitespace(const std::string& str);

bool fileExists(const std::string& path);
bool isDirectory(const std::string& path);
bool isExecutable(const std::string& path);
std::string getAbsolutePath(const std::string& path);
std::string expandTilde(const std::string& path);
std::string normalizePath(const std::string& path);
std::string getFileName(const std::string& path);
std::string getDirectoryName(const std::string& path);
std::string getFileExtension(const std::string& path);
size_t getFileSize(const std::string& path);

bool createDirectory(const std::string& path);
bool createDirectories(const std::string& path);

std::string readFile(const std::string& path);
bool writeFile(const std::string& path, const std::string& content);

std::string getHomeDirectory();
std::string getCurrentWorkingDirectory();

namespace Colors {
extern const std::string RESET;
extern const std::string BOLD;
extern const std::string DIM;
extern const std::string UNDERLINE;
extern const std::string BLINK;
extern const std::string REVERSE;
extern const std::string HIDDEN;

// Regular colors
extern const std::string BLACK;
extern const std::string RED;
extern const std::string GREEN;
extern const std::string YELLOW;
extern const std::string BLUE;
extern const std::string MAGENTA;
extern const std::string CYAN;
extern const std::string WHITE;

// Bright colors
extern const std::string BRIGHT_BLACK;
extern const std::string BRIGHT_RED;
extern const std::string BRIGHT_GREEN;
extern const std::string BRIGHT_YELLOW;
extern const std::string BRIGHT_BLUE;
extern const std::string BRIGHT_MAGENTA;
extern const std::string BRIGHT_CYAN;
extern const std::string BRIGHT_WHITE;

// Background colors
extern const std::string BG_BLACK;
extern const std::string BG_RED;
extern const std::string BG_GREEN;
extern const std::string BG_YELLOW;
extern const std::string BG_BLUE;
extern const std::string BG_MAGENTA;
extern const std::string BG_CYAN;
extern const std::string BG_WHITE;
}  // namespace Colors

// Utility functions for printing
void printError(const std::string& message);
void printWarning(const std::string& message);
void printSuccess(const std::string& message);
void printInfo(const std::string& message);
void printDebug(const std::string& message);

}  // namespace Utils

#endif  // UTILS_H