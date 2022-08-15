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

    size_t instIndex;
};

StackFrame::StackFrame(BigInt val, uint32_t instIndex)
    : val(std::move(val))
    , instIndex(instIndex)
{}

void add(BigInt &num, BigInt addend) {
    num += addend;
}

bool divide(BigInt &num, BigInt divisor) {
    BigInt rem = num % divisor;
    BigInt result = num / divisor;
    num = result;
    return rem == 0;
}

void multiply(BigInt &num, BigInt factor) {
    num *= factor;
}

bool subtract(BigInt &num, BigInt subtrahend) {
    if (subtrahend > num) {
        return false;
    }
    num -= subtrahend;
    return true;
}

} // anonymous namespace

std::optional<BigInt> getResult(const BytecodeModule &bytecode, BigInt initialVal) {
    std::vector<StackFrame> frames = { { initialVal, 0 } };
    std::optional<BigInt> val = initialVal;
    uint32_t instIndex = 0;

    auto getByte = [&] {
        return bytecode[instIndex++];
    };

    auto getLongValue = [&] {
        BigInt value = 0;
        uint8_t byte = 0x80;
        while (byte & 0x80) {
            byte = getByte();
            value <<= 7;
            value |= (byte & 0x7F);
        }
        return value;
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
            add(*val, getByte());
            break;

        case OpCode::AddLong:
            add(*val, getLongValue());
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
            if (!divide(*val, getByte())) {
                val = std::nullopt;
            }
            break;

        case OpCode::DivFailLong:
            if (!divide(*val, getLongValue())) {
                val = std::nullopt;
            }
            break;

        case OpCode::DivFloor:
            divide(*val, getByte());
            break;

        case OpCode::DivFloorLong:
            divide(*val, getLongValue());
            break;

        case OpCode::Equal:
            if (*val != getByte()) {
                val = std::nullopt;
            }
            break;

        case OpCode::EqualLong:
            if (*val != getLongValue()) {
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

        case OpCode::Mult:
            multiply(*val, getByte());
            break;

        case OpCode::MultLong:
            multiply(*val, getLongValue());
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
            if (!subtract(*val, getByte())) {
                val = std::nullopt;
            }
            break;

        case OpCode::SubLong:
            if (!subtract(*val, getLongValue())) {
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
