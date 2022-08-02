#pragma once

#include "bytecode.hpp"

#include <boost/multiprecision/cpp_int.hpp>

#include <optional>

namespace unacpp {

using BigInt = boost::multiprecision::cpp_int;

template <typename NumberType>
std::optional<NumberType> getResult(const BytecodeModule &bytecode, NumberType initialVal);

} // namespace unacpp
