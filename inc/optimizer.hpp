//
//  Copyright 2022 Sam Coppini
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
//

#pragma once

#include "program.hpp"

namespace unacpp {

ProgramMap optimizePrograms(ProgramMap programs, const std::string &programName);

} // namespace unacpp
