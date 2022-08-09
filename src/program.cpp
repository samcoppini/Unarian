//
//  Copyright 2022 Sam Coppini
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
//

#include "program.hpp"

namespace unacpp {

AddProgram::AddProgram(uint32_t amount)
    : amount_(amount)
{}

uint32_t AddProgram::getAmount() const {
    return amount_;
}

DivideProgram::DivideProgram(uint32_t divisor, Remainder remainder)
    : divisor_(divisor)
    , remainder_(remainder)
{}

uint32_t DivideProgram::getDivisor() const {
    return divisor_;
}

DivideProgram::Remainder DivideProgram::getRemainderBehavior() const {
    return remainder_;
}

EqualProgram::EqualProgram(uint32_t amount)
    : amount_(amount)
{}

uint32_t EqualProgram::getAmount() const {
    return amount_;
}

FuncCall::FuncCall(std::string_view funcName, FilePosition pos)
    : funcName_(funcName)
    , pos_(pos)
{}

const std::string &FuncCall::getFuncName() const {
    return funcName_;
}

const FilePosition &FuncCall::getPos() const {
    return pos_;
}

MultiplyProgram::MultiplyProgram(uint32_t amount)
    : amount_(amount)
{}

uint32_t MultiplyProgram::getAmount() const {
    return amount_;
}

SubtractProgram::SubtractProgram(uint32_t amount)
    : amount_(amount)
{}

uint32_t SubtractProgram::getAmount() const {
    return amount_;
}

Branch::Branch(std::vector<Instruction> instructions)
    : instructions_(std::move(instructions))
{}

std::span<const Instruction> Branch::getInstructions() const {
    return std::span{instructions_.begin(), instructions_.size()};
}

Program::Program(std::vector<Branch> branches)
    : branches_(std::move(branches))
{}

std::span<const Branch> Program::getBranches() const {
    return std::span{branches_.begin(), branches_.size()};
}

} // namespace unacpp
