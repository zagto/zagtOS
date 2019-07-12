#ifndef LOGGER_HPP
#define LOGGER_HPP

#include <common/inttypes.hpp>
#include <interrupts/RegisterState.hpp>

struct BootInfo;

namespace log {
    class Logger {
    public:
        void init(const BootInfo *bootInfo);
        Logger operator<<(char character);
        Logger operator<<(const char *string);
        Logger operator<<(usize value);
        Logger operator<<(volatile void *pointer);
        Logger operator<<(const void *pointer);
        Logger operator<<(void *pointer);
        Logger operator<<(const RegisterState &registerState);
    };

    extern Logger Log;
    static const char EndLine = '\n';
}

#endif
