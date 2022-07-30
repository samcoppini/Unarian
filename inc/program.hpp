#pragma once

#include <unordered_map>
#include <span>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

namespace unacpp {

class DebugPrint {};

class Decrement {};

class Increment {};

class FuncCall {
private:
    std::string funcName_;

public:
    FuncCall(std::string_view funcName);

    const std::string &getFuncName() const;
};

using Instruction = std::variant<
    DebugPrint,
    Decrement,
    Increment,
    FuncCall
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
