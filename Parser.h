#ifndef PARSER_H
#define PARSER_H

#include <string>
#include <vector>

#include "Command.h"

class Parser {
   private:
    std::vector<std::string> tokenize(const std::string& input);

   public:
    Parser();
    ~Parser();

    Command parse(const std::string& input);
    bool isEmpty(const std::string& input);
    std::string trim(const std::string& str);
};

#endif