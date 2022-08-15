//
//  Copyright 2022 Sam Coppini
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
//

#pragma once

#include "bigint.hpp"
#include "position.hpp"

#include <span>
#include <string_view>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

namespace unacpp {

class DebugPrint {};

class AddProgram {
private:
    BigInt amount_;

public:
    AddProgram(BigInt amount);

    const BigInt &getAmount() const;
};

class DivideProgram {
public:
    enum class Remainder {
        Fail,
        Floor,
    };

private:
    BigInt divisor_;

    Remainder remainder_;

public:
    DivideProgram(BigInt divisor, Remainder remainder);

    const BigInt &getDivisor() const;

    Remainder getRemainderBehavior() const;
};

class EqualProgram {
private:
    BigInt amount_;

public:
    EqualProgram(BigInt amount);

    const BigInt &getAmount() const;
};

class FuncCall {
private:
    std::string funcName_;

    FilePosition pos_;

public:
    FuncCall(std::string_view funcName, FilePosition pos);

    const std::string &getFuncName() const;

    const FilePosition &getPos() const;
};

class MultiplyProgram {
private:
    BigInt amount_;

public:
    MultiplyProgram(BigInt amount);

    const BigInt &getAmount() const;
};

class NotProgram {};

class SubtractProgram {
private:
    BigInt amount_;

public:
    SubtractProgram(BigInt amount);

    const BigInt &getAmount() const;
};

using Instruction = std::variant<
    AddProgram,
    DebugPrint,
    DivideProgram,
    EqualProgram,
    FuncCall,
    MultiplyProgram,
    NotProgram,
    SubtractProgram
>;

class Branch {
private:
    std::vector<Instruction> instructions_;

public:
    Branch(std::vector<Instruction> instructions);

    std::span<const Instruction> getInstructions() const;
};

class Program {
private:
    std::vector<Branch> branches_;

public:
    Program(std::vector<Branch> branches);

    std::span<const Branch> getBranches() const;
};

using ProgramMap = std::unordered_map<std::string, Program>;

} // namespace unacpp
