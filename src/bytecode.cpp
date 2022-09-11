//
//  Copyright 2022 Sam Coppini
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
//

#include "bytecode.hpp"

#include <sstream>

namespace unacpp {

namespace {

using FuncFailureMap = std::unordered_map<std::string, bool>;

using ConstantMap = std::unordered_map<BigInt, uint16_t>;

struct ProgramReference {
    uint32_t byteIndex;

    std::string_view funcName;
};

void replacePlaceholderAddress(std::vector<uint8_t> &bytecode, uint32_t replaceIndex, uint32_t address) {
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
            std::holds_alternative<ModEqualProgram>(inst) ||
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
    std::vector<uint8_t> &bytecode,
    const ProgramMap &programs,
    const Branch &branch,
    std::vector<ProgramReference> &unresolvedReferences,
    FuncFailureMap funcsFail,
    ConstantMap &constants,
    bool lastBranch)
{
    auto instructions = branch.getInstructions();
    std::vector<uint32_t> nextBranchReferences;

    auto addPlaceholderAddress = [&] {
        bytecode.insert(bytecode.end(), 4, 255);
    };

    auto addValue = [&] (BigInt val) {
        uint16_t index;

        auto constantIter = constants.find(val);
        if (constantIter != constants.end()) {
            index = constantIter->second;
        }
        else {
            index = static_cast<uint16_t>(constants.size());
            constants[val] = index;
        }

        bytecode.push_back((index & 0xFF00) >> 8);
        bytecode.push_back((index & 0x00FF) >> 0);
    };

    for (size_t i = 0; i < instructions.size(); i++) {
        auto &inst = instructions[i];
        bool lastInst = (i == instructions.size() - 1);

        auto addFailureCheck = [&] {
            if (lastBranch) {
                if (!lastInst) {
                    bytecode.push_back(OpCode::RetOnFailure);
                }
            }
            else {
                bytecode.push_back(OpCode::JumpOnFailure);
                nextBranchReferences.push_back(static_cast<uint32_t>(bytecode.size()));
                addPlaceholderAddress();
            }
        };

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
            else {
                bytecode.push_back(OpCode::DivFail);
                addValue(div->getDivisor());
                addFailureCheck();
            }
        }
        else if (std::holds_alternative<NotProgram>(inst)) {
            bytecode.push_back(OpCode::Not);
        }
        else if (auto eq = std::get_if<EqualProgram>(&inst); eq) {
            bytecode.push_back(OpCode::Equal);
            addValue(eq->getAmount());
            addFailureCheck();
        }
        else if (auto modEq = std::get_if<ModEqualProgram>(&inst); modEq) {
            bytecode.push_back(OpCode::ModEqual);
            addValue(modEq->getAmount());
            addValue(modEq->getModulo());
            addFailureCheck();
        }
        else if (auto sub = std::get_if<SubtractProgram>(&inst); sub) {
            if (sub->getAmount() == 1) {
                bytecode.push_back(OpCode::Dec);
            }
            else {
                bytecode.push_back(OpCode::Sub);
                addValue(sub->getAmount());
            }
            addFailureCheck();
        }
        else if (auto call = std::get_if<FuncCall>(&inst); call) {
            bool callCanFail = funcCallCanFail(programs, call->getFuncName(), funcsFail);

            if (lastInst && (!callCanFail || lastBranch)) {
                bytecode.push_back(OpCode::TailCall);
            }
            else {
                bytecode.push_back(OpCode::Call);
            }
            unresolvedReferences.emplace_back(static_cast<uint32_t>(bytecode.size()), call->getFuncName());
            addPlaceholderAddress();

            if (callCanFail) {
                if (!lastBranch) {
                    bytecode.push_back(OpCode::JumpOnFailure);
                    nextBranchReferences.push_back(static_cast<uint32_t>(bytecode.size()));
                    addPlaceholderAddress();
                }
                else if (!lastInst) {
                    bytecode.push_back(OpCode::RetOnFailure);
                }
            }
        }
        else if (std::holds_alternative<DebugPrint>(inst)) {
            bytecode.push_back(OpCode::Print);
        }
    }

    bytecode.push_back(OpCode::Ret);

    for (auto ref: nextBranchReferences) {
        replacePlaceholderAddress(bytecode, ref, static_cast<uint32_t>(bytecode.size()));
    }
}

void generateProgram(
    std::vector<uint8_t> &bytecode,
    const ProgramMap &programs,
    const Program &program,
    std::vector<ProgramReference> &unresolvedReferences,
    FuncFailureMap &funcsFail,
    ConstantMap &constants)
{
    auto branches = program.getBranches();

    for (size_t i = 0; i < branches.size(); i++) {
        generateBranch(bytecode, programs, branches[i], unresolvedReferences, funcsFail, constants, i + 1 == branches.size());
    }
}

enum class ArgType {
    Constant,
    Address,
};

std::vector<ArgType> argumentType(OpCode opcode) {
    switch (opcode) {
        case OpCode::Add:
        case OpCode::DivFail:
        case OpCode::DivFloor:
        case OpCode::Equal:
        case OpCode::Mult:
        case OpCode::Sub:
            return { ArgType::Constant };

        case OpCode::Call:
        case OpCode::JumpOnFailure:
        case OpCode::TailCall:
            return { ArgType::Address };

        case OpCode::ModEqual:
            return { ArgType::Constant, ArgType::Constant };

        default:
            return {};
    }
}

std::string_view opcodeName(OpCode opcode) {
    switch (opcode) {
        case OpCode::Add:           return "ADD";
        case OpCode::Call:          return "CALL";
        case OpCode::Dec:           return "DEC";
        case OpCode::DivFail:       return "DIV_FAIL";
        case OpCode::DivFloor:      return "DIV_FLOOR";
        case OpCode::Equal:         return "EQ";
        case OpCode::Inc:           return "INC";
        case OpCode::JumpOnFailure: return "FAIL_JMP";
        case OpCode::ModEqual:      return "MOD_EQ";
        case OpCode::Mult:          return "MULT";
        case OpCode::Not:           return "NOT";
        case OpCode::Print:         return "PRINT";
        case OpCode::Ret:           return "RET";
        case OpCode::RetOnFailure:  return "FAIL_RET";
        case OpCode::Sub:           return "SUB";
        case OpCode::TailCall:      return "TAIL_CALL";
        default:                    return "ERROR";
    }
}

} // anonymous namespace

BytecodeModule generateBytecode(const ProgramMap &programs, const std::string &mainName) {
    std::vector<uint8_t> instructions;
    std::vector<ProgramReference> programReferences;
    std::unordered_map<std::string_view, uint32_t> programStarts;
    FuncFailureMap funcsFail;
    ConstantMap constantsMap;

    generateProgram(instructions, programs, programs.at(mainName), programReferences, funcsFail, constantsMap);

    for (auto &[progName, program]: programs) {
        if (mainName != progName) {
            programStarts[progName] = static_cast<uint32_t>(instructions.size());
            generateProgram(instructions, programs, program, programReferences, funcsFail, constantsMap);
        }
    }

    for (auto [index, funcName]: programReferences) {
        replacePlaceholderAddress(instructions, index, programStarts.at(funcName));
    }

    std::vector<BigInt> constants(constantsMap.size());
    for (auto &[constant, index]: constantsMap) {
        constants[index] = constant;
    }

    return { instructions, constants };
}

std::string bytecodeToString(const BytecodeModule &bytecode) {
    std::stringstream stream;
    auto &[instructions, constants] = bytecode;

    for (size_t i = 0; i < instructions.size(); i++) {
        stream << i << ": " << opcodeName(static_cast<OpCode>(instructions[i]));

        auto argTypes = argumentType(static_cast<OpCode>(instructions[i]));

        for (auto argType: argTypes) {
            if (argType == ArgType::Address) {
                uint32_t address = 0;
                address |= instructions[++i] << 24;
                address |= instructions[++i] << 16;
                address |= instructions[++i] <<  8;
                address |= instructions[++i] <<  0;
                stream << " " << address;
            }
            else if (argType == ArgType::Constant) {
                uint16_t index = 0;
                index |= instructions[++i] << 8;
                index |= instructions[++i] << 0;
                stream << " " << constants[index];
            }
        }

        stream << '\n';
    }

    return stream.str();
}

} // namespace unacpp
