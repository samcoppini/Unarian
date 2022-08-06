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
    uint16_t curAdd = 0;
    uint16_t curSub = 0;

    std::vector<Instruction> insts;

    auto addSub = [&] {
        if (curSub != 0) {
            insts.push_back(SubtractProgram{curSub});
            curSub = 0;
        }
    };

    auto addAdd = [&] {
        if (curAdd != 0) {
            insts.push_back(AddProgram{curAdd});
            curAdd = 0;
        }
    };

    for (auto &inst: branch.getInstructions()) {
        std::visit([&] (auto &&arg) {
            using T = std::decay_t<decltype(arg)>;
            if constexpr (std::is_same_v<T, AddProgram>) {
                addSub();
                auto newAdd = curAdd + arg.getAmount();
                if (newAdd > UINT16_MAX) {
                    curAdd = UINT16_MAX;
                    addAdd();
                    newAdd -= UINT16_MAX;
                }
                curAdd = newAdd;
            }
            else if constexpr (std::is_same_v<T, SubtractProgram>) {
                if (curAdd > 0) {
                    if (curAdd >= arg.getAmount()) {
                        curAdd -= arg.getAmount();
                    }
                    else {
                        curSub = arg.getAmount() - curAdd;
                        curAdd = 0;
                    }
                }
                else {
                    auto newSub = curSub + arg.getAmount();
                    if (newSub > UINT16_MAX) {
                        curSub = UINT16_MAX;
                        addSub();
                        newSub -= UINT16_MAX;
                    }
                    curSub = newSub;
                }
            }
            else {
                addAdd();
                addSub();
                insts.push_back(inst);
            }
        }, inst);
    }

    addAdd();
    addSub();

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

std::optional<uint32_t> checkMultiply(const Program &program, const std::string &funcName) {
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

std::optional<std::pair<uint32_t, DivideProgram::Remainder>> checkDivision(const Program &program, const std::string &funcName) {
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
