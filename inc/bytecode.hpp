#pragma once

#include "program.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace unacpp {

enum OpCode: uint8_t {
    Call,
    DecJump,
    DecRet,
    Inc,
    Jump,
    JumpOnFailure,
    Print,
    Ret,
    RetOnFailure
};

using BytecodeModule = std::vector<uint8_t>;

BytecodeModule generateBytecode(const ProgramMap &program, const Program &expr, bool debugMode);

std::string bytecodeToString(const BytecodeModule &bytecode);

} // namespace unacpp
