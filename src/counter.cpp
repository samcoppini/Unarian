#include "counter.hpp"

#include <iostream>

namespace unacpp {

Counter::Counter()
    : value_(0)
{}

Counter::Counter(uint64_t val)
    : value_(val)
{}

void Counter::increment() {
    value_++;
}

void Counter::add(uint32_t val) {
    value_ += val;
}

bool Counter::decrement() {
    if (value_ == 0) {
        return false;
    }
    value_--;
    return true;
}

bool Counter::sub(uint32_t val) {
    if (val > value_) {
        return false;
    }
    value_ -= val;
    return true;
}

void Counter::output() {
    std::cout << value_ << '\n';
}

} // namespace unacpp
