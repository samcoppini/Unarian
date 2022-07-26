#include "optimizer.hpp"

#include <optional>

namespace unacpp {

namespace {

bool canInline(const Program &program) {
    auto branches = program.getBranches();
    if (branches.size() > 1) {
        return false;
    }

    for (auto &inst: branches[0].getInstructions()) {
        if (!std::holds_alternative<Decrement>(inst) &&
            !std::holds_alternative<Increment>(inst) &&
            !std::holds_alternative<DebugPrint>(inst))
        {
            return false;
        }
    }

    return true;
}

Program inlineProgram(const Program &program, const ProgramMap &programs);

Branch inlineBranch(const Branch &branch, const ProgramMap &programs) {
    std::vector<Instruction> insts;

    for (auto &inst: branch.getInstructions()) {
        if (auto *func = std::get_if<FuncCall>(&inst); func && programs.count(func->getFuncName())) {
            auto toInline = programs.at(func->getFuncName()).getBranches()[0];
            for (auto &inlineInst: toInline.getInstructions()) {
                insts.push_back(inlineInst);
            }
        }
        else if (auto *progInst = std::get_if<Program>(&inst); progInst) {
            insts.push_back(inlineProgram(*progInst, programs));
        }
        else {
            insts.push_back(inst);
        }
    }

    return Branch{insts};
}

Program inlineProgram(const Program &program, const ProgramMap &programs) {
    std::vector<Branch> branches;

    for (auto &branch: program.getBranches()) {
        branches.push_back(inlineBranch(branch, programs));
    }

    return Program{branches};
}

ProgramMap inlinePrograms(const ProgramMap &programs) {
    ProgramMap optimized = programs;

    bool keepInlining = true;
    ProgramMap inlinablePrograms;

    while (keepInlining) {
        keepInlining = false;

        for (auto it = optimized.begin(); it != optimized.end(); ) {
            if (canInline(it->second)) {
                inlinablePrograms.insert(*it);
                it = optimized.erase(it);
                keepInlining = true;
            }
            else {
                ++it;
            }
        }

        if (!keepInlining) {
            break;
        }

        for (auto &[name, prog]: optimized) {
            prog = inlineProgram(prog, inlinablePrograms);
        }
    }

    optimized.insert(inlinablePrograms.begin(), inlinablePrograms.end());

    return optimized;
}

} // anonymous namespace

Program optimizeProgram(const ProgramMap &programs, const Program &program) {
    return inlineProgram(program, programs);
}

ProgramMap optimizePrograms(const ProgramMap &programs) {
    return inlinePrograms(programs);
}

} // namespace unacpp
