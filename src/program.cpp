#include "program.hpp"

namespace unacpp {

FuncCall::FuncCall(std::string_view funcName)
    : funcName_(funcName)
{}

const std::string &FuncCall::getFuncName() const {
    return funcName_;
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
