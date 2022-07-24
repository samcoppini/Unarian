#pragma once

#include <string_view>
#include <unordered_map>
#include <variant>
#include <vector>
#include <span>

namespace unacpp {

class Branch;

class DebugPrint {};

class Decrement {};

class Increment {};

class FuncCall {
private:
    std::string_view funcName_;

public:
    FuncCall(std::string_view funcName);

    std::string_view getFuncName() const;
};

class Program {
private:
    std::vector<Branch> branches_;

public:
    Program(std::vector<Branch> branches);

    ~Program();

    std::span<const Branch> getBranches() const;
};

using Instruction = std::variant<
    DebugPrint,
    Decrement,
    Increment,
    FuncCall,
    Program
>;

class Branch {
private:
    std::vector<Instruction> instructions_;

public:
    Branch(std::vector<Instruction> instructions);

    std::span<const Instruction> getInstructions() const;
};

using ProgramMap = std::unordered_map<std::string_view, Program>;

} // namespace unacpp
