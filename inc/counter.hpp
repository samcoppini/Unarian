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

    void add(uint32_t val);

    void multiply(uint32_t val);

    bool decrement();

    bool sub(uint32_t val);

    void output();
};

} // namespace unacpp
