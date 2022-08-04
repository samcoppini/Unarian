#include "interpreter.hpp"
#include "bytecode.hpp"

#include <iostream>

namespace unacpp {

namespace {

template <typename NumberType>
struct StackFrame {
    StackFrame(NumberType val, uint32_t instIndex);

    NumberType val;

    size_t instIndex;
};

template <typename NumberType>
StackFrame<NumberType>::StackFrame(NumberType val, uint32_t instIndex)
    : val(val)
    , instIndex(instIndex)
{}

void add(uint64_t &num, uint64_t addend) {
    num += addend;
}

void add(BigInt &num, BigInt addend) {
    num += addend;
}

bool divide(BigInt &num, uint64_t divisor) {
    BigInt rem = num % divisor;
    BigInt result = num / divisor;
    num = result;
    return rem == 0;
}

bool divide(uint64_t &num, uint64_t divisor) {
    auto rem = num % divisor;
    num /= divisor;
    return rem == 0;
}

void multiply(uint64_t &num, uint64_t factor) {
    num *= factor;
}

void multiply(BigInt &num, uint64_t factor) {
    num *= factor;
}

bool subtract(uint64_t &num, uint64_t subtrahend) {
    if (subtrahend > num) {
        return false;
    }
    num -= subtrahend;
    return true;
}

bool subtract(BigInt &num, uint64_t subtrahend) {
    if (subtrahend > num) {
        return false;
    }
    num -= subtrahend;
    return true;
}

} // anonymous namespace

template <typename NumberType>
std::optional<NumberType> getResult(const BytecodeModule &bytecode, NumberType initialVal) {
    std::vector<StackFrame<NumberType>> frames;
    std::optional<NumberType> val = initialVal;
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
            add(*val, getValue());
            break;

        case OpCode::Call: {
            auto newInst = getAddress();
            frames.emplace_back(*val, instIndex);
            instIndex = newInst;
            break;
        }

        case OpCode::DecJump: {
            auto jumpIndex = getAddress();

            if (!subtract(*val, 1)) {
                if (frames.empty()) {
                    val = initialVal;
                }
                else {
                    val = frames.back().val;
                }
                instIndex = jumpIndex;
            }
            break;
        }

        case OpCode::DecRet:
            if (!subtract(*val, 1)) {
                val = std::nullopt;
                if (frames.empty()) {
                    return val;
                }
                else {
                    instIndex = frames.back().instIndex;
                    frames.pop_back();
                }
            }
            break;

        case OpCode::DivFloor:
            divide(*val, getValue());
            break;

        case OpCode::DivJump: {
            auto divisor = getValue();
            auto jumpIndex = getAddress();

            if (!divide(*val, divisor)) {
                if (frames.empty()) {
                    val = initialVal;
                }
                else {
                    val = frames.back().val;
                }
                instIndex = jumpIndex;
            }
            break;
        }

        case OpCode::DivRet:
            if (!divide(*val, getValue())) {
                val = std::nullopt;
                if (frames.empty()) {
                    return val;
                }
                else {
                    instIndex = frames.back().instIndex;
                    frames.pop_back();
                }
            }
            break;

        case OpCode::Inc:
            add(*val, 1);
            break;

        case OpCode::Jump:
            instIndex = getAddress();
            break;

        case OpCode::JumpOnFailure: {
            auto jumpIndex = getAddress();

            if (val == std::nullopt) {
                if (frames.empty()) {
                    val = initialVal;
                }
                else {
                    val = frames.back().val;
                }
                instIndex = jumpIndex;
            }
            break;
        }

        case OpCode::Mult:
            multiply(*val, getValue());
            break;

        case OpCode::Not:
            if (*val == 0) {
                val = 1;
            }
            else {
                val = 0;
            }
            break;

        case OpCode::NotEqualJump: {
            auto eq = getValue();
            auto jumpIndex = getAddress();

            if (*val != eq) {
                if (frames.empty()) {
                    val = initialVal;
                }
                else {
                    val = frames.back().val;
                }
                instIndex = jumpIndex;
            }

            break;
        }

        case OpCode::NotEqualRet: {
            auto eq = getValue();

            if (*val != eq) {
                val = std::nullopt;
                if (frames.empty()) {
                    return val;
                }
                else {
                    instIndex = frames.back().instIndex;
                    frames.pop_back();
                }
            }

            break;
        }

        case OpCode::Print:
            std::cout << *val << '\n';
            break;

        case OpCode::Ret:
            if (frames.empty()) {
                return val;
            }
            else {
                instIndex = frames.back().instIndex;
                frames.pop_back();
            }
            break;

        case OpCode::RetOnFailure:
            if (val == std::nullopt) {
                if (frames.empty()) {
                    return val;
                }
                else {
                    instIndex = frames.back().instIndex;
                    frames.pop_back();
                }
            }
            break;

        case OpCode::SubJump: {
            auto toSub = getValue();
            auto jumpIndex = getAddress();

            if (!subtract(*val, toSub)) {
                if (frames.empty()) {
                    val = initialVal;
                }
                else {
                    val = frames.back().val;
                }
                instIndex = jumpIndex;
            }
            break;
        }

        case OpCode::SubRet:
            if (!subtract(*val, getValue())) {
                val = std::nullopt;
                if (frames.empty()) {
                    return val;
                }
                else {
                    instIndex = frames.back().instIndex;
                    frames.pop_back();
                }
            }
            break;

        case OpCode::TailCall:
            instIndex = getAddress();
            if (frames.empty()) {
                initialVal = *val;
            }
            else {
                frames.back().val = *val;
            }
            break;
        }
    }
}

template std::optional<uint64_t> getResult<uint64_t>(const BytecodeModule &bytecode, uint64_t initialVal);
template std::optional<BigInt> getResult<BigInt>(const BytecodeModule &bytecode, BigInt initialVal);

} // namespace unacpp
