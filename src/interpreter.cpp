#include "interpreter.hpp"

#include <type_traits>

namespace unacpp {

namespace {

struct StackFrame {
    StackFrame(const Program &program, Counter val);

    const Program &program;

    Counter counter;

    size_t branchIndex;

    size_t instIndex;
};

StackFrame::StackFrame(const Program &program, Counter counter)
    : program(program)
    , counter(counter)
    , branchIndex(0)
    , instIndex(0)
{}

} // anonymous namespace

RunResult getResult(const ProgramMap &programs, const Program &expr, Counter counter, bool debugMode) {
    std::vector<StackFrame> frames = { { expr, counter } };

    while (true) {
        auto &frame = frames.back();
        auto &program = frame.program;

        auto &bi = frame.branchIndex;
        auto branches = program.getBranches();
        if (bi >= branches.size()) {
            frames.pop_back();
            if (frames.empty()) {
                return std::nullopt;
            }
            frames.back().branchIndex++;
            frames.back().instIndex = 0;
            counter = frames.back().counter;
            continue;
        }

        auto &branch = branches[bi];
        auto &ii = frame.instIndex;
        auto instructions = branch.getInstructions();
        if (ii >= instructions.size()) {
            frames.pop_back();
            if (frames.empty()) {
                return counter;
            }
            frames.back().instIndex++;
            continue;
        }

        auto &instruction = instructions[ii];
        if (std::holds_alternative<Increment>(instruction)) {
            counter.increment();
            ii++;
        }
        else if (std::holds_alternative<Decrement>(instruction)) {
            if (counter.decrement()) {
                ii++;
            }
            else {
                counter = frame.counter;
                ii = 0;
                bi++;
            }
        }
        else if (auto *func = std::get_if<FuncCall>(&instruction); func) {
            auto programIt = programs.find(func->getFuncName());
            if (programIt == programs.end()) {
                return RuntimeError{"Attempted to run non-existent program " + std::string{func->getFuncName()}};
            }
            frames.emplace_back(programIt->second, counter);
        }
        else if (auto *prog = std::get_if<Program>(&instruction); prog) {
            frames.emplace_back(*prog, counter);
        }
        else if (std::holds_alternative<DebugPrint>(instruction)) {
            if (debugMode) {
                counter.output();
            }
            ii++;
        }
    }
}

} // namespace unacpp
