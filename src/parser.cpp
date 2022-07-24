#include "parser.hpp"
#include "program.hpp"
#include "token.hpp"

#include <cctype>
#include <optional>

using namespace std::string_literals;

namespace unacpp {

namespace {

enum class TokenType {
    Branch,
    EndGroup,
    Name,
    StartGroup,
};

TokenType getType(const Token &token) {
    if (token.content == "|") {
        return TokenType::Branch;
    }
    else if (token.content == "{") {
        return TokenType::StartGroup;
    }
    else if (token.content == "}") {
        return TokenType::EndGroup;
    }
    else {
        return TokenType::Name;
    }
}

std::optional<Token> getIf(const std::vector<Token> &tokens, size_t &i, TokenType type) {
    if (i >= tokens.size() || getType(tokens[i]) != type) {
        return std::nullopt;
    }
    return tokens[i++];
}

std::optional<Program> parseProgram(const std::vector<Token> &, size_t &, ParseErrors &);

Branch parseBranch(const std::vector<Token> &tokens, size_t &i, ParseErrors &errors) {
    std::vector<Instruction> instructions;
    std::optional<Token> token;

    auto nextToken = [&] {
        return ((token = getIf(tokens, i, TokenType::Name)) != std::nullopt) ||
               ((token = getIf(tokens, i, TokenType::StartGroup)) != std::nullopt);
    };

    while (nextToken()) {
        if (token->content == "{") {
            auto program = parseProgram(tokens, --i, errors);
            if (program != std::nullopt) {
                instructions.emplace_back(std::move(*program));
            }
        }
        else {
            instructions.push_back(FuncCall{token->content});
        }
    }

    return Branch{instructions};
}

std::optional<Program> parseProgram(const std::vector<Token> &tokens, size_t &i, ParseErrors &errors) {
    auto startGroup = getIf(tokens, i, TokenType::StartGroup);

    if (startGroup == std::nullopt) {
        FilePosition pos;
        if (i >= tokens.size()) {
            pos = tokens[i - 1].pos;
            pos.col += tokens[i - 1].content.size();
        }
        else {
            pos = tokens[i].pos;
        }
        errors.emplace_back(pos, "Expected a {");
        return std::nullopt;
    }

    std::vector<Branch> branches;

    do {
        branches.push_back(parseBranch(tokens, i, errors));
    } while (getIf(tokens, i, TokenType::Branch) != std::nullopt);

    if (getIf(tokens, i, TokenType::EndGroup) == std::nullopt) {
        errors.emplace_back(startGroup->pos, "No matching } for {");
        return std::nullopt;
    }

    return Program{std::move(branches)};
}

void parseNamedProgram(const std::vector<Token> &tokens, size_t &i, ProgramMap &programs, ParseErrors &errors) {
    auto nameToken = getIf(tokens, i, TokenType::Name);

    if (nameToken == std::nullopt) {
        errors.emplace_back(tokens[i].pos, "Expected a name!");
        i++;
        return;
    }

    auto program = parseProgram(tokens, i, errors);

    if (program != std::nullopt) {
        if (programs.count(nameToken->content)) {
            errors.emplace_back(nameToken->pos, "Cannot redefine " + std::string{nameToken->content});
        }
        else {
            programs.insert({nameToken->content, *program});
        }
    }
}

} // anonymous namespace

FileParseResult parseFile(std::string_view fileContent) {
    ProgramMap programs;
    ParseErrors errors;

    auto tokens = getTokens(fileContent);
    size_t i = 0;

    programs.insert({"!", Program{{Branch{{DebugPrint{}}}}}});
    programs.insert({"-", Program{{Branch{{Decrement{}}}}}});
    programs.insert({"+", Program{{Branch{{Increment{}}}}}});

    while (i < tokens.size()) {
        parseNamedProgram(tokens, i, programs, errors);
    }

    if (errors.empty()) {
        return programs;
    }
    else {
        return errors;
    }
}

ExpressionParseResult parseExpression(std::string_view expression) {
    std::vector<Branch> branches;
    ParseErrors errors;

    auto tokens = getTokens(expression);
    size_t i = 0;

    do {
        branches.push_back(parseBranch(tokens, i, errors));
    } while (getIf(tokens, i, TokenType::Branch) != std::nullopt);

    if (i < tokens.size()) {
        errors.emplace_back(tokens[i].pos, "Unexpected " + std::string{tokens[i].content} + " encountered");
    }

    if (errors.empty()) {
        return Program{std::move(branches)};
    }
    else {
        return errors;
    }
}

} // namespace unacpp
