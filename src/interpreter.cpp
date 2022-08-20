//
//  Copyright 2022 Sam Coppini
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
//

#include "interpreter.hpp"
#include "bytecode.hpp"

#include <iostream>

namespace unacpp {

namespace {

struct StackFrame {
    StackFrame(BigInt val, uint32_t instIndex);

    BigInt val;

    uint32_t instIndex;
};

StackFrame::StackFrame(BigInt val, uint32_t instIndex)
    : val(std::move(val))
    , instIndex(instIndex)
{}

void add(BigInt &num, const BigInt &addend) {
    num += addend;
}

bool divide(BigInt &num, const BigInt &divisor) {
    BigInt rem = num % divisor;
    BigInt result = num / divisor;
    num = result;
    return rem == 0;
}

void multiply(BigInt &num, const BigInt &factor) {
    num *= factor;
}

bool subtract(BigInt &num, const BigInt &subtrahend) {
    if (subtrahend > num) {
        return false;
    }
    num -= subtrahend;
    return true;
}

} // anonymous namespace

std::optional<BigInt> getResult(const BytecodeModule &bytecodeModule, BigInt initialVal) {
    auto &[bytecode, constants] = bytecodeModule;
    std::vector<StackFrame> frames = { { initialVal, 0 } };
    std::optional<BigInt> val = initialVal;
    uint32_t instIndex = 0;

    auto getByte = [&] {
        return bytecode[instIndex++];
    };

    auto getValue = [&] () -> const BigInt & {
        uint16_t index = 0;
        index |= getByte() << 8;
        index |= getByte() << 0;
        return constants[index];
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

        case OpCode::Dec:
            if (!subtract(*val, 1)) {
                val = std::nullopt;
            }
            break;

        case OpCode::DivFail:
            if (!divide(*val, getValue())) {
                val = std::nullopt;
            }
            break;

        case OpCode::DivFloor:
            divide(*val, getValue());
            break;

        case OpCode::Equal:
            if (*val != getValue()) {
                val = std::nullopt;
            }
            break;

        case OpCode::Inc:
            add(*val, 1);
            break;

        case OpCode::JumpOnFailure: {
            auto jumpIndex = getAddress();

            if (val == std::nullopt) {
                val = frames.back().val;
                instIndex = jumpIndex;
            }
            break;
        }

        case OpCode::ModEqual: {
            auto cmp = getValue();
            auto modulo = getValue();
            if (*val % modulo != cmp) {
                val = std::nullopt;
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

        case OpCode::Print:
            std::cout << *val << '\n';
            break;

        case OpCode::Ret:
            if (frames.size() == 1) {
                return val;
            }
            else {
                instIndex = frames.back().instIndex;
                frames.pop_back();
            }
            break;

        case OpCode::RetOnFailure:
            if (val == std::nullopt) {
                if (frames.size() == 1) {
                    return val;
                }
                else {
                    instIndex = frames.back().instIndex;
                    frames.pop_back();
                }
            }
            break;

        case OpCode::Sub:
            if (!subtract(*val, getValue())) {
                val = std::nullopt;
            }
            break;

        case OpCode::TailCall:
            instIndex = getAddress();
            frames.back().val = *val;
            break;
        }
    }
}

} // namespace unacpp
