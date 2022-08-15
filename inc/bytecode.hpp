//
//  Copyright 2022 Sam Coppini
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
//

#pragma once

#include "program.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace unacpp {

enum OpCode: uint8_t {
    Add,
    AddLong,
    Call,
    Dec,
    DivFail,
    DivFailLong,
    DivFloor,
    DivFloorLong,
    Equal,
    EqualLong,
    Inc,
    JumpOnFailure,
    Mult,
    MultLong,
    Not,
    Print,
    Ret,
    RetOnFailure,
    Sub,
    SubLong,
    TailCall,
};

using BytecodeModule = std::vector<uint8_t>;

BytecodeModule generateBytecode(const ProgramMap &program, const std::string &mainName, bool debugMode);

std::string bytecodeToString(const BytecodeModule &bytecode);

} // namespace unacpp
