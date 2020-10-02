#pragma once

#include <common/inttypes.hpp>

class Logger {
public:
    Logger operator<<(char character);
    Logger operator<<(const char *string);
    Logger operator<<(size_t value);
};

extern Logger cout;
static const char endl = '\n';
