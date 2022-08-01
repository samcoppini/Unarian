#include "optimizer.hpp"

#include <type_traits>
#include <optional>

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

bool inlinePrograms(ProgramMap &programs) {
    ProgramMap inlinablePrograms;

    for (auto it = programs.begin(); it != programs.end(); ) {
        if (canInline(it->second)) {
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

    programs.insert(inlinablePrograms.begin(), inlinablePrograms.end());
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

void findMultiplies(ProgramMap &programs) {
    for (auto &[name, prog]: programs) {
        auto factor = checkMultiply(prog, name);
        if (factor != std::nullopt) {
            prog = Program{{Branch{{MultiplyProgram{*factor}}}}};
        }
    }
}

} // anonymous namespace

ProgramMap optimizePrograms(ProgramMap programs) {
    bool keepOptimizing = true;

    while (keepOptimizing) {
        keepOptimizing = inlinePrograms(programs);
        if (keepOptimizing) {
            condenseMath(programs);
            findMultiplies(programs);
        }
    }

    return programs;
}

} // namespace unacpp
