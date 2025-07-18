#ifndef UTILS_H
#define UTILS_H

#include <string>
#include <vector>

namespace Utils {

// String manipulation functions
std::string trim(const std::string& str);
std::vector<std::string> split(const std::string& str, char delimiter);
bool endsWith(const std::string& str, const std::string& suffix);
std::string normalizePath(const std::string& path);

// File and directory functions
bool fileExists(const std::string& path);
bool isDirectory(const std::string& path);
std::string expandTilde(const std::string& path);
std::string getHomeDirectory();
std::string getCurrentWorkingDirectory();

// Color constants for output
namespace Colors {
extern const std::string RESET;
extern const std::string RED;
extern const std::string GREEN;
extern const std::string YELLOW;
extern const std::string CYAN;
extern const std::string DIM;
}  // namespace Colors

// Utility functions for printing
void printError(const std::string& message);
void printWarning(const std::string& message);
void printSuccess(const std::string& message);
void printInfo(const std::string& message);
void printDebug(const std::string& message);

}  // namespace Utils

#endif