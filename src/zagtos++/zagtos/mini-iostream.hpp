#pragma once

#include <cstdint>

namespace mio {

struct OutputStream {
    OutputStream &operator<<(const char *string);
    OutputStream &operator<<(uint16_t value);
    OutputStream &operator<<(uint32_t value);
    OutputStream &operator<<(uint64_t value);
    OutputStream &operator<<(char value);
    OutputStream &operator<<(bool value);
    OutputStream &operator<<(volatile void *pointer);
    OutputStream &operator<<(const void *pointer);
    OutputStream &operator<<(void *pointer);
};

extern OutputStream cout;
extern OutputStream cerr;
static constexpr char endl = '\n';

}
