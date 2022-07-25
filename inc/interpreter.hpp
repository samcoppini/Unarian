#pragma once

#include "bytecode.hpp"
#include "counter.hpp"

#include <optional>

namespace unacpp {

std::optional<Counter> getResult(const BytecodeModule &bytecode, Counter initialVal);

} // namespace unacpp
