#include "bytecode.hpp"

#include <sstream>

namespace unacpp {

namespace {

using FuncFailureMap = std::unordered_map<std::string, bool>;

struct ProgramReference {
    uint32_t byteIndex;

    std::string_view funcName;
};

void replacePlaceholderAddress(BytecodeModule &bytecode, uint32_t replaceIndex, uint32_t address) {
    bytecode[replaceIndex + 0] = (address >> 24) & 0xFF;
    bytecode[replaceIndex + 1] = (address >> 16) & 0xFF;
    bytecode[replaceIndex + 2] = (address >>  8) & 0xFF;
    bytecode[replaceIndex + 3] = (address >>  0) & 0xFF;
}

bool funcCallCanFail(const ProgramMap &programs, const std::string &funcName, FuncFailureMap &funcsFail);

bool branchCanFail(const ProgramMap &programs, const Branch &branch, std::unordered_map<std::string, bool> &funcsFail) {
    for (auto &inst: branch.getInstructions()) {
        if (std::holds_alternative<SubtractProgram>(inst) ||
            std::holds_alternative<EqualProgram>(inst) ||
            (std::holds_alternative<DivideProgram>(inst) &&
             std::get<DivideProgram>(inst).getRemainderBehavior() == DivideProgram::Remainder::Fail))
        {
            return true;
        }
        else if (auto func = std::get_if<FuncCall>(&inst); func) {
            if (funcCallCanFail(programs, func->getFuncName(), funcsFail)) {
                return true;
            }
        }
    }

    return false;
}

bool funcCallCanFail(const ProgramMap &programs, const std::string &funcName, FuncFailureMap &funcsFail) {
    auto funcIt = funcsFail.find(funcName);
    if (funcIt != funcsFail.end()) {
        return funcIt->second;
    }

    funcsFail[funcName] = true;

    auto &program = programs.at(funcName);
    for (auto &branch: program.getBranches()) {
        if (!branchCanFail(programs, branch, funcsFail)) {
            funcsFail[funcName] = false;
            return false;
        }
    }

    return true;
}

void generateBranch(
    BytecodeModule &bytecode,
    const ProgramMap &programs,
    const Branch &branch,
    std::vector<ProgramReference> &unresolvedReferences,
    FuncFailureMap funcsFail,
    bool lastBranch,
    bool debugMode)
{
    auto instructions = branch.getInstructions();
    std::vector<uint32_t> nextBranchReferences;

    auto addPlaceholderAddress = [&] {
        bytecode.insert(bytecode.end(), 4, 255);
    };

    auto addValue = [&] (uint16_t val) {
        bytecode.push_back((val & 0xFF00) >> 8);
        bytecode.push_back((val & 0x00FF) >> 0);
    };

    for (size_t i = 0; i < instructions.size(); i++) {
        auto &inst = instructions[i];
        bool lastInst = (i == instructions.size() - 1);

        if (auto add = std::get_if<AddProgram>(&inst); add) {
            if (add->getAmount() == 1) {
                bytecode.push_back(OpCode::Inc);
            }
            else {
                bytecode.push_back(OpCode::Add);
                addValue(add->getAmount());
            }
        }
        else if (auto mult = std::get_if<MultiplyProgram>(&inst); mult) {
            bytecode.push_back(OpCode::Mult);
            addValue(mult->getAmount());
        }
        else if (auto div = std::get_if<DivideProgram>(&inst); div) {
            if (div->getRemainderBehavior() == DivideProgram::Remainder::Floor) {
                bytecode.push_back(OpCode::DivFloor);
                addValue(div->getDivisor());
            }
            else if (lastBranch) {
                bytecode.push_back(OpCode::DivRet);
                addValue(div->getDivisor());
            }
            else {
                bytecode.push_back(OpCode::DivJump);
                addValue(div->getDivisor());
                nextBranchReferences.push_back(bytecode.size());
                addPlaceholderAddress();
            }
        }
        else if (std::holds_alternative<NotProgram>(inst)) {
            bytecode.push_back(OpCode::Not);
        }
        else if (auto eq = std::get_if<EqualProgram>(&inst); eq) {
            if (lastBranch) {
                bytecode.push_back(OpCode::NotEqualRet);
                addValue(eq->getAmount());
            }
            else {
                bytecode.push_back(OpCode::NotEqualJump);
                addValue(eq->getAmount());
                nextBranchReferences.push_back(bytecode.size());
                addPlaceholderAddress();
            }
        }
        else if (auto sub = std::get_if<SubtractProgram>(&inst); sub) {
            if (sub->getAmount() == 1) {
                if (lastBranch) {
                    bytecode.push_back(OpCode::DecRet);
                }
                else {
                    bytecode.push_back(OpCode::DecJump);
                    nextBranchReferences.push_back(bytecode.size());
                    addPlaceholderAddress();
                }
            }
            else {
                if (lastBranch) {
                    bytecode.push_back(OpCode::SubRet);
                    addValue(sub->getAmount());
                }
                else {
                    bytecode.push_back(OpCode::SubJump);
                    addValue(sub->getAmount());
                    nextBranchReferences.push_back(bytecode.size());
                    addPlaceholderAddress();
                }
            }
        }
        else if (auto call = std::get_if<FuncCall>(&inst); call) {
            bool callCanFail = funcCallCanFail(programs, call->getFuncName(), funcsFail);

            if (lastInst && (!callCanFail || lastBranch)) {
                bytecode.push_back(OpCode::TailCall);
            }
            else {
                bytecode.push_back(OpCode::Call);
            }
            unresolvedReferences.emplace_back(bytecode.size(), call->getFuncName());
            addPlaceholderAddress();

            if (callCanFail) {
                if (!lastBranch) {
                    bytecode.push_back(OpCode::JumpOnFailure);
                    nextBranchReferences.push_back(bytecode.size());
                    addPlaceholderAddress();
                }
                else if (!lastInst) {
                    bytecode.push_back(OpCode::RetOnFailure);
                }
            }
        }
        else if (std::holds_alternative<DebugPrint>(inst) && debugMode) {
            bytecode.push_back(OpCode::Print);
        }
    }

    bytecode.push_back(OpCode::Ret);

    for (auto ref: nextBranchReferences) {
        replacePlaceholderAddress(bytecode, ref, bytecode.size());
    }
}

void generateProgram(
    BytecodeModule &bytecode,
    const ProgramMap &programs,
    const Program &program,
    std::vector<ProgramReference> &unresolvedReferences,
    FuncFailureMap &funcsFail,
    bool debugMode)
{
    auto branches = program.getBranches();

    for (size_t i = 0; i < branches.size(); i++) {
        generateBranch(bytecode, programs, branches[i], unresolvedReferences, funcsFail, i + 1 == branches.size(), debugMode);
    }
}

std::vector<size_t> argumentSizes(OpCode opcode) {
    switch (opcode) {
        case OpCode::Add:
        case OpCode::DivFloor:
        case OpCode::DivRet:
        case OpCode::Mult:
        case OpCode::NotEqualRet:
        case OpCode::SubRet:
            return { 2 };

        case OpCode::Call:
        case OpCode::DecJump:
        case OpCode::JumpOnFailure:
        case OpCode::TailCall:
            return { 4 };

        case OpCode::DivJump:
        case OpCode::NotEqualJump:
        case OpCode::SubJump:
            return { 2, 4 };

        default:
            return {};
    }
}

std::string_view opcodeName(OpCode opcode) {
    switch (opcode) {
        case OpCode::Add:           return "ADD";
        case OpCode::Call:          return "CALL";
        case OpCode::DecJump:       return "DEC_JMP";
        case OpCode::DecRet:        return "DEC_RET";
        case OpCode::DivFloor:      return "DIV_FLOOR";
        case OpCode::DivJump:       return "DIV_JUMP";
        case OpCode::DivRet:        return "DIV_RET";
        case OpCode::Inc:           return "INC";
        case OpCode::JumpOnFailure: return "FAIL_JMP";
        case OpCode::Mult:          return "MULT";
        case OpCode::Not:           return "NOT";
        case OpCode::NotEqualJump:  return "NOT_EQ_JMP";
        case OpCode::NotEqualRet:   return "NOT_EQ_RET";
        case OpCode::Print:         return "PRINT";
        case OpCode::Ret:           return "RET";
        case OpCode::RetOnFailure:  return "FAIL_RET";
        case OpCode::SubJump:       return "SUB_JMP";
        case OpCode::SubRet:        return "SUB_RET";
        case OpCode::TailCall:      return "TAIL_CALL";
        default:                    return "ERROR";
    }
}

} // anonymous namespace

BytecodeModule generateBytecode(const ProgramMap &programs, const std::string &mainName, bool debugMode) {
    BytecodeModule bytecode;
    std::vector<ProgramReference> programReferences;
    std::unordered_map<std::string_view, uint32_t> programStarts;
    FuncFailureMap funcsFail;

    generateProgram(bytecode, programs, programs.at(mainName), programReferences, funcsFail, debugMode);

    for (auto &[progName, program]: programs) {
        if (mainName != progName) {
            programStarts[progName] = bytecode.size();
            generateProgram(bytecode, programs, program, programReferences, funcsFail, debugMode);
        }
    }

    for (auto [index, funcName]: programReferences) {
        replacePlaceholderAddress(bytecode, index, programStarts.at(funcName));
    }

    return bytecode;
}

std::string bytecodeToString(const BytecodeModule &bytecode) {
    std::stringstream stream;

    for (size_t i = 0; i < bytecode.size(); i++) {
        stream << i << ": " << opcodeName(static_cast<OpCode>(bytecode[i]));

        auto argSizes = argumentSizes(static_cast<OpCode>(bytecode[i]));

        for (auto argSize: argSizes) {
            stream << ' ';

            uint32_t argument = 0;
            for (size_t j = 0; j < argSize; j++) {
                argument = (argument << 8) | bytecode[++i];
            }

            stream << argument;
        }

        stream << '\n';
    }

    return stream.str();
}

} // namespace unacpp
