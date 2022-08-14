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

template <typename NumberType>
void add(NumberType &num, uint64_t addend) {
    num += addend;
}

template <typename NumberType>
bool divide(NumberType &num, uint64_t divisor) {
    NumberType rem = num % divisor;
    NumberType result = num / divisor;
    num = result;
    return rem == 0;
}

template <typename NumberType>
void multiply(NumberType &num, uint64_t factor) {
    num *= factor;
}

template <typename NumberType>
bool subtract(NumberType &num, uint64_t subtrahend) {
    if (subtrahend > num) {
        return false;
    }
    num -= subtrahend;
    return true;
}

} // anonymous namespace

template <typename NumberType>
std::optional<NumberType> getResult(const BytecodeModule &bytecode, NumberType initialVal) {
    std::vector<StackFrame<NumberType>> frames = { { initialVal, 0 } };
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

template std::optional<uint64_t> getResult<uint64_t>(const BytecodeModule &bytecode, uint64_t initialVal);
template std::optional<BigInt> getResult<BigInt>(const BytecodeModule &bytecode, BigInt initialVal);

} // namespace unacpp
