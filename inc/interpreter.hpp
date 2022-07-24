#pragma once

#include "counter.hpp"
#include "program.hpp"

#include <optional>
#include <string>

namespace unacpp {

struct RuntimeError {
    std::string error;
};

using RunResult = std::variant<RuntimeError, std::optional<Counter>>;

RunResult getResult(const ProgramMap &programs, const Program &expr, Counter initialVal, bool debugMode);

} // namespace unacpp
