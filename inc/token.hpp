#pragma once

#include "position.hpp"

#include <string_view>
#include <vector>

namespace unacpp {

struct Token {
    std::string_view content;

    FilePosition pos;
};

std::vector<Token> getTokens(std::string_view fileContent);

} // namespace unacpp
