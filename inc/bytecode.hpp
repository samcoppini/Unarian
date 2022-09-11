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

// Defines the instructions used by the VM. These opcodes can be followed by
// arguments which are either 4-byte instruction addresses, or 2-byte indexes
// into an array of constants.
enum OpCode: uint8_t {
    // ADD [constant]
    // Adds the constant to the current value.
    Add,

    // CALL [address]
    // Pushes the address of the next instruction and the current value onto the
    // stack, then continues execution from the given address.
    Call,

    // DEC
    // Subtracts 1 from the current value, and enter a failed state if that
    // causes the value to be negative.
    Dec,

    // DIV_FAIL [constant]
    // Divides the current value by the constant. If it does not divide evenly,
    // then the program enters a failed state.
    DivFail,

    // DIV_FLOOR [constant]
    // Divides the current value by the constant, discarding the fractional part
    // if it doesn't divide evenly.
    DivFloor,

    // EQ [constant]
    // Checks if the current value is equal to the constant, and if not,
    // enter a failed state.
    Equal,

    // INC
    // Adds 1 to the current value.
    Inc,

    // FAIL_JMP [address]
    // If the program is in a failed state, then the failed state is cleared,
    // the value is restored to what it was when the function was first called,
    // and execution continues from the address. Otherwise, it does nothing.
    JumpOnFailure,

    // MOD_EQ [constant] [constant]
    // Checks if the value modulo the first constant is not equal to the second
    // constant, then the program enters a failed state.
    ModEqual,

    // MULT [constant]
    // Multiplies the current value by the constant.
    Mult,

    // NOT
    // If the current value is zero, change the current value to one. Otherwise,
    // the current value is changed to zero.
    Not,

    // PRINT
    // Prints the current value.
    Print,

    // RET
    // Returns execution to the calling function.
    Ret,

    // FAIL_RET
    // If the program is in a failed state, then execution returns to the
    // calling function, retaining the failed state.
    RetOnFailure,

    // SUB [constant]
    // Subtracts the constant to the current value, and enter a failed state if
    // that causes the value to be negative.
    Sub,

    // TAIL_CALL [address]
    // Replaces the value on top of the stack with the current value, but does
    // not otherwise increase the stack size. Continues execution from the
    // address.
    TailCall,
};

struct BytecodeModule {
    // The instructions for the program
    std::vector<uint8_t> instructions;

    // The list of all the constants used by the program
    std::vector<BigInt> constants;
};

BytecodeModule generateBytecode(const ProgramMap &program, const std::string &mainName);

std::string bytecodeToString(const BytecodeModule &bytecode);

} // namespace unacpp
