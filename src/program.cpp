//
//  Copyright 2022 Sam Coppini
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
//

#include "program.hpp"

namespace unacpp {

AddProgram::AddProgram(BigInt amount)
    : amount_(std::move(amount))
{}

const BigInt &AddProgram::getAmount() const {
    return amount_;
}

DivideProgram::DivideProgram(BigInt divisor, Remainder remainder)
    : divisor_(std::move(divisor))
    , remainder_(remainder)
{}

const BigInt &DivideProgram::getDivisor() const {
    return divisor_;
}

DivideProgram::Remainder DivideProgram::getRemainderBehavior() const {
    return remainder_;
}

EqualProgram::EqualProgram(BigInt amount)
    : amount_(std::move(amount))
{}

const BigInt &EqualProgram::getAmount() const {
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

ModEqualProgram::ModEqualProgram(BigInt amount, BigInt modulo)
    : amount_(std::move(amount))
    , modulo_(std::move(modulo))
{}

const BigInt &ModEqualProgram::getAmount() const {
    return amount_;
}

const BigInt &ModEqualProgram::getModulo() const {
    return modulo_;
}

MultiplyProgram::MultiplyProgram(BigInt amount)
    : amount_(std::move(amount))
{}

const BigInt &MultiplyProgram::getAmount() const {
    return amount_;
}

SubtractProgram::SubtractProgram(BigInt amount)
    : amount_(std::move(amount))
{}

const BigInt &SubtractProgram::getAmount() const {
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
