#include "Parser.h"

#include "Utils.h"

Parser::Parser() {}

Parser::~Parser() {}

Command Parser::parse(const std::string& input) {
    Command cmd;
    std::string processedInput = Utils::trim(input);

    if (processedInput.empty()) return cmd;

    std::vector<std::string> tokens = tokenize(processedInput);
    if (tokens.empty()) return cmd;

    cmd.name = tokens[0];
    for (size_t i = 1; i < tokens.size(); ++i) {
        cmd.arguments.push_back(tokens[i]);
    }

    return cmd;
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

        if (!inQuotes && !inSingleQuotes && (c == ' ' || c == '\t')) {
            if (!current.empty()) {
                tokens.push_back(current);
                current.clear();
            }
            continue;
        }

        current += c;
    }

    if (!current.empty()) tokens.push_back(current);
    return tokens;
}

bool Parser::isEmpty(const std::string& input) { return Utils::trim(input).empty(); }