#pragma once

#include <unordered_map>
#include <span>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

namespace unacpp {

class DebugPrint {};

class AddProgram {
private:
    uint32_t amount_;

public:
    AddProgram(uint32_t amount);

    uint32_t getAmount() const;
};

class DivideProgram {
public:
    enum class Remainder {
        Fail,
        Floor,
    };

private:
    uint32_t divisor_;

    Remainder remainder_;

public:
    DivideProgram(uint32_t divisor, Remainder remainder);

    uint32_t getDivisor() const;

    Remainder getRemainderBehavior() const;
};

class EqualProgram {
private:
    uint32_t amount_;

public:
    EqualProgram(uint32_t amount);

    uint32_t getAmount() const;
};

class FuncCall {
private:
    std::string funcName_;

public:
    FuncCall(std::string_view funcName);

    const std::string &getFuncName() const;
};

class MultiplyProgram {
private:
    uint32_t amount_;

public:
    MultiplyProgram(uint32_t amount);

    uint32_t getAmount() const;
};

class NotProgram {};

class SubtractProgram {
private:
    uint32_t amount_;

public:
    SubtractProgram(uint32_t amount);

    uint32_t getAmount() const;
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
