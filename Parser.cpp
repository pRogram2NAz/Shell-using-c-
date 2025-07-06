#include "Parser.h"
#include "Utils.h"
#include <sstream>
#include <algorithm>

Parser::Parser() {}

Parser::~Parser() {}

Pipeline Parser::parse(const std::string& input) {
    Pipeline pipeline;
    std::string processedInput = expandVariables(trim(input));
    
    if (isEmpty(processedInput)) {
        return pipeline;
    }
    
    // Split by pipes
    std::vector<std::string> commandStrings = Utils::split(processedInput, '|');
    
    for (auto& cmdStr : commandStrings) {
        cmdStr = trim(cmdStr);
        if (cmdStr.empty()) continue;
        
        Command cmd;
        std::vector<std::string> tokens = tokenize(cmdStr);
        
        if (tokens.empty()) continue;
        
        cmd.name = tokens[0];
        
        // Parse arguments and redirections
        for (size_t i = 1; i < tokens.size(); ++i) {
            if (tokens[i] == "<") {
                parseRedirection(cmd, tokens, i);
            } else if (tokens[i] == ">" || tokens[i] == ">>") {
                parseRedirection(cmd, tokens, i);
            } else if (tokens[i] == "&") {
                cmd.isBackground = true;
            } else {
                cmd.arguments.push_back(tokens[i]);
            }
        }
        
        // Check for background process (& at the end)
        parseBackground(cmd, tokens);
        
        pipeline.commands.push_back(cmd);
    }
    
    return pipeline;
}

std::vector<std::string> Parser::tokenize(const std::string& input) {
    std::vector<std::string> tokens;
    std::string current;
    bool inQuotes = false;
    bool inSingleQuotes = false;
    bool escaped = false;
    
    for (size_t i = 0; i < input.length(); ++i) {
        char c = input[i];
        
        if (escaped) {
            current += c;
            escaped = false;
            continue;
        }
        
        if (c == '\\' && !inSingleQuotes) {
            escaped = true;
            continue;
        }
        
        if (c == '"' && !inSingleQuotes) {
            inQuotes = !inQuotes;
            continue;
        }
        
        if (c == '\'' && !inQuotes) {
            inSingleQuotes = !inSingleQuotes;
            continue;
        }
        
        if (!inQuotes && !inSingleQuotes) {
            if (c == ' ' || c == '\t') {
                if (!current.empty()) {
                    tokens.push_back(current);
                    current.clear();
                }
                continue;
            }
            
            // Handle special characters
            if (c == '<' || c == '>' || c == '|' || c == '&') {
                if (!current.empty()) {
                    tokens.push_back(current);
                    current.clear();
                }
                
                // Handle >> redirection
                if (c == '>' && i + 1 < input.length() && input[i + 1] == '>') {
                    tokens.push_back(">>");
                    ++i;
                } else {
                    tokens.push_back(std::string(1, c));
                }
                continue;
            }
        }
        
        current += c;
    }
    
    if (!current.empty()) {
        tokens.push_back(current);
    }
    
    return tokens;
}

void Parser::parseRedirection(Command& cmd, const std::vector<std::string>& tokens, size_t& index) {
    if (index + 1 >= tokens.size()) {
        throw std::runtime_error("Syntax error: expected filename after redirection");
    }
    
    const std::string& op = tokens[index];
    const std::string& filename = tokens[index + 1];
    
    if (op == "<") {
        cmd.inputRedirection = filename;
    } else if (op == ">") {
        cmd.outputRedirection = filename;
        cmd.appendOutput = false;
    } else if (op == ">>") {
        cmd.outputRedirection = filename;
        cmd.appendOutput = true;
    }
    
    ++index; // Skip the filename token
}

void Parser::parseBackground(Command& cmd, const std::vector<std::string>& tokens) {
    if (!tokens.empty() && tokens.back() == "&") {
        cmd.isBackground = true;
    }
}

std::string Parser::expandVariables(const std::string& input) {
    std::string result = input;
    size_t pos = 0;
    
    while ((pos = result.find('$', pos)) != std::string::npos) {
        size_t start = pos + 1;
        size_t end = start;
        
        // Find the end of the variable name
        if (start < result.length() && result[start] == '{') {
            // ${VAR} format
            start++;
            end = result.find('}', start);
            if (end == std::string::npos) {
                pos = start;
                continue;
            }
        } else {
            // $VAR format
            while (end < result.length() && 
                   (std::isalnum(result[end]) || result[end] == '_')) {
                end++;
            }
        }
        
        if (end > start) {
            std::string varName = result.substr(start, end - start);
            const char* varValue = getenv(varName.c_str());
            std::string replacement = varValue ? std::string(varValue) : "";
            
            size_t replaceStart = (result[pos + 1] == '{') ? pos : pos;
            size_t replaceEnd = (result[pos + 1] == '{') ? end + 1 : end;
            
            result.replace(replaceStart, replaceEnd - replaceStart, replacement);
            pos = replaceStart + replacement.length();
        } else {
            pos++;
        }
    }
    
    // Handle tilde expansion
    if (result[0] == '~') {
        const char* home = getenv("HOME");
        if (home) {
            result.replace(0, 1, std::string(home));
        }
    }
    
    return result;
}

bool Parser::isEmpty(const std::string& input) {
    return trim(input).empty();
}

std::string Parser::trim(const std::string& str) {
    return Utils::trim(str);
}