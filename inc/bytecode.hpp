#pragma once

#include "program.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace unacpp {

enum OpCode: uint8_t {
    Add,
    Call,
    DecJump,
    DecRet,
    Inc,
    Jump,
    JumpOnFailure,
    Mult,
    Print,
    Ret,
    RetOnFailure,
    SubJump,
    SubRet,
    TailCall,
};

using BytecodeModule = std::vector<uint8_t>;

BytecodeModule generateBytecode(const ProgramMap &program, const std::string &mainName, bool debugMode);

std::string bytecodeToString(const BytecodeModule &bytecode);

} // namespace unacpp
