#pragma once

#include "position.hpp"
#include "program.hpp"

#include <string>

namespace unacpp {

struct ParseError {
    FilePosition pos;

    std::string message;
};

using ParseErrors = std::vector<ParseError>;

using FileParseResult = std::variant<ProgramMap, ParseErrors>;

using ExpressionParseResult = std::variant<Program, ParseErrors>;

FileParseResult parseFile(std::string_view fileContent);

ExpressionParseResult parseExpression(std::string_view expression);

} // namespace unacpp
