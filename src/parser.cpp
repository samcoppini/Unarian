#include "parser.hpp"
#include "program.hpp"
#include "token.hpp"

#include <cctype>
#include <optional>

using namespace std::string_literals;

namespace unacpp {

Parser::TokenType Parser::getType(const Token &token) {
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

std::optional<Token> Parser::getIf(TokenType type) {
    if (index_ >= tokens_.size() || getType(tokens_[index_]) != type) {
        return std::nullopt;
    }
    return tokens_[index_++];
}

std::string Parser::getAnonymousProgramName() const {
    return std::to_string(programs_.size()) + " ";
}

Branch Parser::parseBranch() {
    std::vector<Instruction> instructions;
    std::optional<Token> token;

    auto nextToken = [&] {
        return ((token = getIf(TokenType::Name)) != std::nullopt) ||
               ((token = getIf(TokenType::StartGroup)) != std::nullopt);
    };

    while (nextToken()) {
        if (token->content == "{") {
            index_--;
            auto program = parseProgram();

            if (program != std::nullopt) {
                auto progName = getAnonymousProgramName();
                programs_.insert({progName, *program});
                instructions.push_back(FuncCall{progName});
            }
        }
        else {
            instructions.push_back(FuncCall{token->content});
        }
    }

    return Branch{instructions};
}

std::optional<Program> Parser::parseProgram() {
    auto startGroup = getIf(TokenType::StartGroup);

    if (startGroup == std::nullopt) {
        FilePosition pos;
        if (index_ >= tokens_.size()) {
            pos = tokens_[index_ - 1].pos;
            pos.col += tokens_[index_ - 1].content.size();
        }
        else {
            pos = tokens_[index_].pos;
        }
        errors_.emplace_back(pos, "Expected a {");
        return std::nullopt;
    }

    std::vector<Branch> branches;

    do {
        branches.push_back(parseBranch());
    } while (getIf(TokenType::Branch) != std::nullopt);

    if (getIf(TokenType::EndGroup) == std::nullopt) {
        errors_.emplace_back(startGroup->pos, "No matching } for {");
        return std::nullopt;
    }

    return Program{std::move(branches)};
}

void Parser::parseExpression(std::string_view expr) {
    tokens_ = getTokens(expr);
    index_ = 0;

    std::vector<Branch> branches;

    do {
        branches.push_back(parseBranch());
    } while (getIf(TokenType::Branch) != std::nullopt);

    if (index_ < tokens_.size()) {
        errors_.emplace_back(tokens_[index_].pos, "Unexpected " + std::string{tokens_[index_].content} + " encountered");
    }

    exprName_ = getAnonymousProgramName();
    programs_.insert({exprName_, Program{branches}});
}

void Parser::parseNamedProgram() {
    auto nameToken = getIf(TokenType::Name);

    if (nameToken == std::nullopt) {
        errors_.emplace_back(tokens_[index_].pos, "Expected a name!");
        index_++;
        return;
    }

    auto program = parseProgram();

    if (program != std::nullopt) {
        if (programs_.count(std::string{nameToken->content})) {
            errors_.emplace_back(nameToken->pos, "Cannot redefine " + std::string{nameToken->content});
        }
        else {
            programs_.insert({std::string{nameToken->content}, *program});
        }
    }
}

void Parser::parseFilePrograms() {
    while (index_ < tokens_.size()) {
        parseNamedProgram();
    }
}

Parser::Parser(std::string_view fileContent, std::string_view expr)
    : tokens_(getTokens(fileContent))
    , index_(0)
{
    programs_.insert({"!", Program{{Branch{{DebugPrint{}}}}}});
    programs_.insert({"-", Program{{Branch{{Decrement{}}}}}});
    programs_.insert({"+", Program{{Branch{{Increment{}}}}}});

    parseFilePrograms();
    parseExpression(expr);
}

const std::string &Parser::getExpressionName() const {
    return exprName_;
}

FileParseResult Parser::getParseResult() const {
    if (errors_.empty()) {
        return programs_;
    }
    else {
        return errors_;
    }
}

} // namespace unacpp
