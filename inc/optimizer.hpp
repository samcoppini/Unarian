#pragma once

#include "program.hpp"

namespace unacpp {

Program optimizeProgram(const ProgramMap &programs, const Program &program);

ProgramMap optimizePrograms(const ProgramMap &programs);

} // namespace unacpp
