//
//  Copyright 2022 Sam Coppini
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
//

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
