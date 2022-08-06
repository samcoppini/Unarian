//
//  Copyright 2022 Sam Coppini
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
//

#pragma once

#include "position.hpp"
#include "program.hpp"
#include "token.hpp"

#include <optional>
#include <string>

namespace unacpp {

struct ParseError {
    FilePosition pos;

    std::string message;
};

using ParseErrors = std::vector<ParseError>;

using FileParseResult = std::variant<ProgramMap, ParseErrors>;

class Parser {
private:
    enum class TokenType {
        Branch,
        EndGroup,
        Name,
        StartGroup,
    };

    std::vector<Token> tokens_;

    ParseErrors errors_;

    size_t index_;

    ProgramMap programs_;

    std::string exprName_;

    static TokenType getType(const Token &token);

    std::optional<Token> getIf(TokenType type);

    std::string getAnonymousProgramName() const;

    Branch parseBranch();

    std::optional<Program> parseProgram();

    void parseExpression(std::string_view expr);

    void parseNamedProgram();

    void parseFilePrograms();

public:
    Parser(std::string_view fileContent, std::string_view expr);

    const std::string &getExpressionName() const;

    FileParseResult getParseResult() const;
};

} // namespace unacpp
