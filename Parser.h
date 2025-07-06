#ifndef PARSER_H
#define PARSER_H

#include <string>
#include <vector>
#include "Command.h"

class Parser {
private:
    std::vector<std::string> tokenize(const std::string& input);
    void parseRedirection(Command& cmd, const std::vector<std::string>& tokens, size_t& index);
    void parseBackground(Command& cmd, const std::vector<std::string>& tokens);
    std::string expandVariables(const std::string& input);
    
public:
    Parser();
    ~Parser();
    
    Pipeline parse(const std::string& input);
    bool isEmpty(const std::string& input);
    std::string trim(const std::string& str);
};

#endif // PARSER_H