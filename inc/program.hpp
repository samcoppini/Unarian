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

class MultiplyProgram {
private:
    uint32_t amount_;

public:
    MultiplyProgram(uint32_t amount);

    uint32_t getAmount() const;
};

class SubtractProgram {
private:
    uint32_t amount_;

public:
    SubtractProgram(uint32_t amount);

    uint32_t getAmount() const;
};

class FuncCall {
private:
    std::string funcName_;

public:
    FuncCall(std::string_view funcName);

    const std::string &getFuncName() const;
};

using Instruction = std::variant<
    AddProgram,
    DebugPrint,
    FuncCall,
    MultiplyProgram,
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
