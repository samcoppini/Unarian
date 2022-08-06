//
//  Copyright 2022 Sam Coppini
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
//

#include "token.hpp"

#include <algorithm>
#include <cctype>

namespace unacpp {

namespace {

std::vector<std::string_view> splitLines(std::string_view fileContent) {
    std::vector<std::string_view> lines;

    size_t index = 0;

    while (index < fileContent.size()) {
        size_t lineEnd = std::min(fileContent.size() - 1, fileContent.find('\n', index));
        lines.push_back(fileContent.substr(index, lineEnd - index + 1));
        index = lineEnd + 1;
    }

    return lines;
}

constexpr const char *whitespace = " \f\n\r\t\v";

} // anonymous namespace

std::vector<Token> getTokens(std::string_view fileContent) {
    std::vector<Token> tokens;

    auto lines = splitLines(fileContent);
    for (size_t i = 0; i < lines.size(); i++) {
        auto commentStart = lines[i].find('#');
        auto line = lines[i].substr(0, commentStart);

        auto tokenStart = line.find_first_not_of(whitespace);

        while (tokenStart < line.size()) {
            FilePosition pos = { i + 1, tokenStart + 1 };
            auto tokenEnd = std::min(line.size() + 1, line.find_first_of(whitespace, tokenStart));
            tokens.emplace_back(line.substr(tokenStart, tokenEnd - tokenStart), pos);
            tokenStart = line.find_first_not_of(whitespace, tokenEnd);
        }
    }

    return tokens;
}

} // namespace unacpp
