//
//  Copyright 2022 Sam Coppini
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
//

#pragma once

#include "bytecode.hpp"

#include <boost/multiprecision/cpp_int.hpp>

#include <optional>

namespace unacpp {

using BigInt = boost::multiprecision::cpp_int;

template <typename NumberType>
std::optional<NumberType> getResult(const BytecodeModule &bytecode, NumberType initialVal);

} // namespace unacpp
