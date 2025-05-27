#ifndef UTILS_HPP
#define UTILS_HPP

#include <cstdint>

namespace utils {
uint16_t getUniqueId() {
    static uint16_t current = 1;
    return current++;
}
}

#endif
