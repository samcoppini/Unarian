#include "program.hpp"

namespace unacpp {

AddProgram::AddProgram(uint32_t amount)
    : amount_(amount)
{}

uint32_t AddProgram::getAmount() const {
    return amount_;
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
