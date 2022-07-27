#include "interpreter.hpp"
#include "bytecode.hpp"

namespace unacpp {

namespace {

struct StackFrame {
    StackFrame(Counter counter, uint32_t instIndex);

    Counter counter;

    size_t instIndex;
};

StackFrame::StackFrame(Counter counter, uint32_t instIndex)
    : counter(counter)
    , instIndex(instIndex)
{}

} // anonymous namespace

std::optional<Counter> getResult(const BytecodeModule &bytecode, Counter initialVal) {
    std::optional<Counter> counter = initialVal;
    std::vector<StackFrame> frames;
    uint32_t instIndex = 0;

    auto getByte = [&] {
        return bytecode[instIndex++];
    };

    auto getValue = [&] {
        uint16_t val = 0;
        val |= getByte() << 8;
        val |= getByte() << 0;
        return val;
    };

    auto getAddress = [&] {
        uint32_t address = 0;
        address |= getByte() << 24;
        address |= getByte() << 16;
        address |= getByte() << 8;
        address |= getByte() << 0;
        return address;
    };

    while (true) {
        switch (getByte()) {
        case OpCode::Add:
            counter->add(getValue());
            break;

        case OpCode::Call: {
            auto newInst = getAddress();
            frames.emplace_back(*counter, instIndex);
            instIndex = newInst;
            break;
        }

        case OpCode::DecJump: {
            auto jumpIndex = getAddress();

            if (!counter->decrement()) {
                if (frames.empty()) {
                    counter = initialVal;
                }
                else {
                    counter = frames.back().counter;
                }
                instIndex = jumpIndex;
            }
            break;
        }

        case OpCode::DecRet:
            if (!counter->decrement()) {
                counter = std::nullopt;
                if (frames.empty()) {
                    return counter;
                }
                else {
                    instIndex = frames.back().instIndex;
                    frames.pop_back();
                }
            }
            break;

        case OpCode::Inc:
            counter->increment();
            break;

        case OpCode::Jump:
            instIndex = getAddress();
            break;

        case OpCode::JumpOnFailure: {
            auto jumpIndex = getAddress();

            if (counter == std::nullopt) {
                if (frames.empty()) {
                    counter = initialVal;
                }
                else {
                    counter = frames.back().counter;
                }
                instIndex = jumpIndex;
            }
            break;
        }

        case OpCode::Print:
            counter->output();
            break;

        case OpCode::Ret:
            if (frames.empty()) {
                return counter;
            }
            else {
                instIndex = frames.back().instIndex;
                frames.pop_back();
            }
            break;

        case OpCode::RetOnFailure:
            if (counter == std::nullopt) {
                if (frames.empty()) {
                    return counter;
                }
                else {
                    instIndex = frames.back().instIndex;
                    frames.pop_back();
                }
            }
            break;

        case OpCode::SubJump: {
            auto val = getValue();
            auto jumpIndex = getAddress();

            if (!counter->sub(val)) {
                if (frames.empty()) {
                    counter = initialVal;
                }
                else {
                    counter = frames.back().counter;
                }
                instIndex = jumpIndex;
            }
            break;
        }

        case OpCode::SubRet:
            if (!counter->sub(getValue())) {
                counter = std::nullopt;
                if (frames.empty()) {
                    return counter;
                }
                else {
                    instIndex = frames.back().instIndex;
                    frames.pop_back();
                }
            }
            break;
        }
    }
}

} // namespace unacpp
