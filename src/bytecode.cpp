#include "bytecode.hpp"

#include <sstream>

namespace unacpp {

namespace {

struct ProgramReference {
    uint32_t byteIndex;

    std::string_view funcName;
};

void generateProgram(
    BytecodeModule &bytecode,
    const Program &program,
    std::vector<ProgramReference> &unresolvedReferences,
    bool debugMode);

void replacePlaceholderAddress(BytecodeModule &bytecode, uint32_t replaceIndex, uint32_t address) {
    bytecode[replaceIndex + 0] = (address >> 24) & 0xFF;
    bytecode[replaceIndex + 1] = (address >> 16) & 0xFF;
    bytecode[replaceIndex + 2] = (address >>  8) & 0xFF;
    bytecode[replaceIndex + 3] = (address >>  0) & 0xFF;
}

void generateBranch(
    BytecodeModule &bytecode,
    const Branch &branch,
    std::vector<ProgramReference> &unresolvedReferences,
    bool lastBranch,
    bool debugMode)
{
    auto instructions = branch.getInstructions();
    std::vector<uint32_t> nextBranchReferences;
    uint32_t curAdd = 0;
    uint32_t curSub = 0;

    auto addPlaceholderAddress = [&] {
        bytecode.insert(bytecode.end(), 4, 255);
    };

    auto addValue = [&] (uint16_t val) {
        bytecode.push_back((val & 0xFF00) >> 8);
        bytecode.push_back((val & 0x00FF) >> 0);
    };

    auto pushAddInstruction = [&] {
        if (curAdd == 1) {
            bytecode.push_back(OpCode::Inc);
        }
        else {
            bytecode.push_back(OpCode::Add);
            addValue(curAdd);
        }
        curAdd = 0;
    };

    auto pushSubInstruction = [&] {
        if (lastBranch) {
            if (curSub == 1) {
                bytecode.push_back(OpCode::DecRet);
            }
            else {
                bytecode.push_back(OpCode::SubRet);
                addValue(curSub);
            }
        }
        else {
            if (curSub == 1) {
                bytecode.push_back(OpCode::DecJump);
            }
            else {
                bytecode.push_back(OpCode::SubJump);
                addValue(curSub);
            }
            nextBranchReferences.push_back(bytecode.size());
            addPlaceholderAddress();
        }
        curSub = 0;
    };

    for (size_t i = 0; i < instructions.size(); i++) {
        auto &inst = instructions[i];
        bool lastInst = (i == instructions.size() - 1);

        if (curSub > 0 && (!std::holds_alternative<Decrement>(inst) || curSub == UINT16_MAX)) {
            pushSubInstruction();
        }

        if (curAdd > 0 && (
            (!std::holds_alternative<Decrement>(inst) && !std::holds_alternative<Increment>(inst)) ||
            (std::holds_alternative<Increment>(inst) && curAdd == UINT16_MAX)))
        {
            pushAddInstruction();
        }

        if (std::holds_alternative<Increment>(inst)) {
            curAdd++;
        }
        else if (std::holds_alternative<Decrement>(inst)) {
            if (curAdd > 0) {
                curAdd--;
            }
            else {
                curSub++;
            }
        }
        else if (auto call = std::get_if<FuncCall>(&inst); call) {
            bytecode.push_back(OpCode::Call);
            unresolvedReferences.emplace_back(bytecode.size(), call->getFuncName());
            addPlaceholderAddress();

            if (!lastBranch) {
                bytecode.push_back(OpCode::JumpOnFailure);
                nextBranchReferences.push_back(bytecode.size());
                addPlaceholderAddress();
            }
            else if (!lastInst) {
                bytecode.push_back(OpCode::RetOnFailure);
            }
        }
        else if (std::holds_alternative<DebugPrint>(inst) && debugMode) {
            bytecode.push_back(OpCode::Print);
        }
    }

    if (curAdd > 0) {
        pushAddInstruction();
    }
    else if (curSub > 0) {
        pushSubInstruction();
    }

    bytecode.push_back(OpCode::Ret);

    for (auto ref: nextBranchReferences) {
        replacePlaceholderAddress(bytecode, ref, bytecode.size());
    }
}

void generateProgram(
    BytecodeModule &bytecode,
    const Program &program,
    std::vector<ProgramReference> &unresolvedReferences,
    bool debugMode)
{
    auto branches = program.getBranches();

    for (size_t i = 0; i < branches.size(); i++) {
        generateBranch(bytecode, branches[i], unresolvedReferences, i + 1 == branches.size(), debugMode);
    }
}

std::vector<size_t> argumentSizes(OpCode opcode) {
    switch (opcode) {
        case OpCode::Add:
        case OpCode::SubRet:
            return { 2 };

        case OpCode::Call:
        case OpCode::DecJump:
        case OpCode::Jump:
        case OpCode::JumpOnFailure:
            return { 4 };

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
        case OpCode::Inc:           return "INC";
        case OpCode::Jump:          return "JMP";
        case OpCode::JumpOnFailure: return "FAIL_JMP";
        case OpCode::Print:         return "PRINT";
        case OpCode::Ret:           return "RET";
        case OpCode::RetOnFailure:  return "FAIL_RET";
        case OpCode::SubJump:       return "SUB_JMP";
        case OpCode::SubRet:        return "SUB_RET";
        default:                    return "ERROR";
    }
}

} // anonymous namespace

BytecodeModule generateBytecode(const ProgramMap &programs, const std::string &mainName, bool debugMode) {
    BytecodeModule bytecode;
    std::vector<ProgramReference> programReferences;
    std::unordered_map<std::string_view, uint32_t> programStarts;

    generateProgram(bytecode, programs.at(mainName), programReferences, debugMode);

    for (auto &[progName, program]: programs) {
        if (mainName != progName) {
            programStarts[progName] = bytecode.size();
            generateProgram(bytecode, program, programReferences, debugMode);
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
