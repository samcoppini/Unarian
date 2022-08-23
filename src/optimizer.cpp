//
//  Copyright 2022 Sam Coppini
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
//

#include "optimizer.hpp"

#include <optional>
#include <type_traits>
#include <utility>

namespace unacpp {

namespace {

bool canInline(const Program &program) {
    auto branches = program.getBranches();
    if (branches.size() > 1) {
        return false;
    }

    for (auto &inst: branches[0].getInstructions()) {
        if (std::holds_alternative<FuncCall>(inst))
        {
            return false;
        }
    }

    return true;
}

Branch inlineBranch(const Branch &branch, const ProgramMap &programs, bool &inlined) {
    std::vector<Instruction> insts;

    for (auto &inst: branch.getInstructions()) {
        if (auto *func = std::get_if<FuncCall>(&inst); func && programs.count(func->getFuncName())) {
            auto toInline = programs.at(func->getFuncName()).getBranches()[0];
            for (auto &inlineInst: toInline.getInstructions()) {
                insts.push_back(inlineInst);
            }
            inlined = true;
        }
        else {
            insts.push_back(inst);
        }
    }

    return Branch{insts};
}

Program inlineProgram(const Program &program, const ProgramMap &programs, bool &inlined) {
    std::vector<Branch> branches;

    for (auto &branch: program.getBranches()) {
        branches.push_back(inlineBranch(branch, programs, inlined));
    }

    return Program{branches};
}

bool inlinePrograms(ProgramMap &programs, const std::string &programName) {
    ProgramMap inlinablePrograms;

    for (auto it = programs.begin(); it != programs.end(); ) {
        if (canInline(it->second) && it->first != programName) {
            inlinablePrograms.insert(*it);
            it = programs.erase(it);
        }
        else {
            ++it;
        }
    }

    bool inlinedProgram = false;

    for (auto &[name, prog]: programs) {
        prog = inlineProgram(prog, inlinablePrograms, inlinedProgram);
    }

    return inlinedProgram;
}

Branch condenseMathBranch(const Branch &branch) {
    BigInt curAdd = 0;
    BigInt curSub = 0;
    BigInt curMul = 1;
    BigInt curDiv = 1;
    std::optional<DivideProgram::Remainder> divType;

    std::vector<Instruction> insts;

    auto pushInstructions = [&] {
        if (curSub > 0) {
            insts.push_back(SubtractProgram{std::move(curSub)});
            curSub = 0;
        }

        if (curDiv != 1) {
            insts.push_back(DivideProgram{std::move(curDiv), *divType});
            curDiv = 1;
            divType = std::nullopt;
        }

        if (curMul != 1) {
            insts.push_back(MultiplyProgram{std::move(curMul)});
            curMul = 1;
        }

        if (curAdd > 0) {
            insts.push_back(AddProgram{std::move(curAdd)});
            curAdd = 0;
        }
    };

    for (auto &inst: branch.getInstructions()) {
        if (auto add = std::get_if<AddProgram>(&inst); add) {
            if (curSub > 0) {
                pushInstructions();
            }
            curAdd += add->getAmount();
        }
        else if (auto sub = std::get_if<SubtractProgram>(&inst); sub) {
            if (curMul != 1 || curDiv != 1) {
                pushInstructions();
            }
            if (curAdd == 0) {
                curSub += sub->getAmount();
            }
            else if (curAdd >= sub->getAmount()) {
                curAdd -= sub->getAmount();
            }
            else {
                curSub = sub->getAmount() - curAdd;
                curAdd = 0;
            }
        }
        else if (auto mul = std::get_if<MultiplyProgram>(&inst); mul) {
            if (mul->getAmount() == 0) {
                curAdd = 0;
                curDiv = 1;
                curMul = 0;
                pushInstructions();
            }
            else {
                if (curSub > 0 || curDiv != 1) {
                    pushInstructions();
                }
                curMul *= mul->getAmount();
                curAdd *= mul->getAmount();
            }
        }
        else if (auto div = std::get_if<DivideProgram>(&inst); div) {
            if (curSub > 0 || curAdd > 0 || curMul != 1) {
                pushInstructions();
            }
            if (divType != std::nullopt && div->getRemainderBehavior() != *divType) {
                pushInstructions();
            }
            divType = div->getRemainderBehavior();
            curDiv *= div->getDivisor();
        }
        else {
            pushInstructions();
            insts.push_back(inst);
        }
    }

    pushInstructions();

    return Branch{insts};
}

void condenseMath(ProgramMap &programs) {
    for (auto &[name, prog]: programs) {
        std::vector<Branch> branches;

        for (auto &branch: prog.getBranches()) {
            branches.emplace_back(condenseMathBranch(branch));
        }

        prog = Program{branches};
    }
}

std::optional<BigInt> checkMultiply(const Program &program, const std::string &funcName) {
    auto branches = program.getBranches();
    if (branches.size() != 2) {
        return std::nullopt;
    }

    auto &identBranch = branches[1];
    if (!identBranch.getInstructions().empty()) {
        return std::nullopt;
    }

    auto &multBranch = branches[0];
    auto multInsts = multBranch.getInstructions();
    if (multInsts.size() != 3 && multInsts.size() != 2) {
        return std::nullopt;
    }

    if (!std::holds_alternative<SubtractProgram>(multInsts[0]) ||
        std::get<SubtractProgram>(multInsts[0]).getAmount() != 1)
    {
        return std::nullopt;
    }

    if (!std::holds_alternative<FuncCall>(multInsts[1]) ||
        std::get<FuncCall>(multInsts[1]).getFuncName() != funcName)
    {
        return std::nullopt;
    }

    if (multInsts.size() == 2) {
        return 0;
    }

    if (!std::holds_alternative<AddProgram>(multInsts[2])) {
        return std::nullopt;
    }

    return std::get<AddProgram>(multInsts[2]).getAmount();
}

bool checkNot(const Program &program) {
    auto branches = program.getBranches();
    if (branches.size() != 2) {
        return false;
    }

    auto firstBranchInsts = branches[0].getInstructions();
    if (firstBranchInsts.size() != 2) {
        return false;
    }

    if (!std::holds_alternative<SubtractProgram>(firstBranchInsts[0]) ||
        std::get<SubtractProgram>(firstBranchInsts[0]).getAmount() != 1)
    {
        return false;
    }

    if (!std::holds_alternative<MultiplyProgram>(firstBranchInsts[1]) ||
        std::get<MultiplyProgram>(firstBranchInsts[1]).getAmount() != 0)
    {
        return false;
    }

    auto secondBranchInsts = branches[1].getInstructions();
    if (secondBranchInsts.size() != 1) {
        return false;
    }

    if (!std::holds_alternative<AddProgram>(secondBranchInsts[0]) ||
        std::get<AddProgram>(secondBranchInsts[0]).getAmount() != 1)
    {
        return false;
    }

    return true;
}

std::optional<uint32_t> checkIfEqual(const Program &program) {
    auto branches = program.getBranches();
    if (branches.size() != 1) {
        return std::nullopt;
    }

    auto branchInsts = branches[0].getInstructions();
    if (branchInsts.size() != 2) {
        return std::nullopt;
    }

    if (!std::holds_alternative<NotProgram>(branchInsts[0]) ||
        !std::holds_alternative<SubtractProgram>(branchInsts[1]) ||
        std::get<SubtractProgram>(branchInsts[1]).getAmount() != 1)
    {
        return std::nullopt;
    }

    return 0;
}

std::optional<std::pair<BigInt, DivideProgram::Remainder>> checkDivision(const Program &program, const std::string &funcName) {
    auto branches = program.getBranches();
    if (branches.size() != 2) {
        return std::nullopt;
    }

    auto firstInsts = branches[0].getInstructions();
    if (firstInsts.size() != 3) {
        return std::nullopt;
    }

    if (!std::holds_alternative<SubtractProgram>(firstInsts[0])) {
        return std::nullopt;
    }

    auto divisor = std::get<SubtractProgram>(firstInsts[0]).getAmount();

    if (!std::holds_alternative<FuncCall>(firstInsts[1]) ||
        std::get<FuncCall>(firstInsts[1]).getFuncName() != funcName)
    {
        return std::nullopt;
    }

    if (!std::holds_alternative<AddProgram>(firstInsts[2]) ||
        std::get<AddProgram>(firstInsts[2]).getAmount() != 1)
    {
        return std::nullopt;
    }

    auto secondInsts = branches[1].getInstructions();
    if (secondInsts.size() != 1) {
        return std::nullopt;
    }

    if (std::holds_alternative<MultiplyProgram>(secondInsts[0]) &&
        std::get<MultiplyProgram>(secondInsts[0]).getAmount() == 0)
    {
        return {{ divisor, DivideProgram::Remainder::Floor }};
    }

    if (std::holds_alternative<EqualProgram>(secondInsts[0]) &&
        std::get<EqualProgram>(secondInsts[0]).getAmount() == 0)
    {
        return {{ divisor, DivideProgram::Remainder::Fail }};
    }

    return std::nullopt;
}

std::optional<std::pair<BigInt, BigInt>> checkModEqual(const Program &program, const std::string &funcName) {
    auto branches = program.getBranches();
    if (branches.size() != 2) {
        return std::nullopt;
    }

    auto firstInsts = branches[0].getInstructions();
    if (firstInsts.size() != 3) {
        return std::nullopt;
    }

    if (!std::holds_alternative<SubtractProgram>(firstInsts[0])) {
        return std::nullopt;
    }

    auto &divisor = std::get<SubtractProgram>(firstInsts[0]).getAmount();

    if (!std::holds_alternative<FuncCall>(firstInsts[1]) ||
        std::get<FuncCall>(firstInsts[1]).getFuncName() != funcName)
    {
        return std::nullopt;
    }

    if (!std::holds_alternative<AddProgram>(firstInsts[2]) ||
        std::get<AddProgram>(firstInsts[2]).getAmount() != divisor)
    {
        return std::nullopt;
    }

    auto secondInsts = branches[1].getInstructions();
    if (secondInsts.size() != 1) {
        return std::nullopt;
    }

    if (std::holds_alternative<EqualProgram>(secondInsts[0]))
    {
        auto equalVal = std::get<EqualProgram>(secondInsts[0]).getAmount();
        return {{ equalVal, divisor }};
    }

    return std::nullopt;
}

void simplifyFunctions(ProgramMap &programs) {
    for (auto &[name, prog]: programs) {
        auto factor = checkMultiply(prog, name);
        if (factor != std::nullopt) {
            prog = Program{{Branch{{MultiplyProgram{*factor}}}}};
            continue;
        }

        auto divide = checkDivision(prog, name);
        if (divide != std::nullopt) {
            auto [divisor, failBehavior] = *divide;
            prog = Program{{Branch{{DivideProgram{divisor, failBehavior}}}}};
            continue;
        }

        auto eq = checkIfEqual(prog);
        if (eq != std::nullopt) {
            prog = Program{{Branch{{EqualProgram{*eq}}}}};
            continue;
        }

        if (checkNot(prog)) {
            prog = Program{{Branch{{NotProgram{}}}}};
            continue;
        }

        auto modEq = checkModEqual(prog, name);
        if (modEq != std::nullopt) {
            auto &[equalVal, divisor] = *modEq;
            prog = Program{{Branch{{ModEqualProgram{equalVal, divisor}}}}};
            continue;
        }
    }
}

} // anonymous namespace

ProgramMap optimizePrograms(ProgramMap programs, const std::string &programName) {
    bool keepOptimizing = true;

    while (keepOptimizing) {
        keepOptimizing = inlinePrograms(programs, programName);
        if (keepOptimizing) {
            condenseMath(programs);
            simplifyFunctions(programs);
        }
    }

    return programs;
}

} // namespace unacpp
