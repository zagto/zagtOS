#ifndef LOGGER_HPP
#define LOGGER_HPP

#include <common/inttypes.hpp>
#include <interrupts/RegisterState.hpp>

struct BootInfo;

class Logger {
public:
    void init(const BootInfo *bootInfo);
    void flush();
    Logger operator<<(char character);
    Logger operator<<(const char *string);
    Logger operator<<(uint32_t value);
    Logger operator<<(uint64_t value);
    Logger operator<<(volatile void *pointer);
    Logger operator<<(const void *pointer);
    Logger operator<<(void *pointer);
    Logger operator<<(const RegisterState &registerState);
};

extern Logger cout;
static const char endl = '\n';

#endif
