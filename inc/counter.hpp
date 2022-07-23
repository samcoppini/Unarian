#pragma once

#include <cstdint>

namespace unacpp {

class Counter {
private:
    uint64_t value_;

public:
    Counter();
    explicit Counter(uint64_t);

    void increment();

    bool decrement();

    void output();
};

} // namespace unacpp
