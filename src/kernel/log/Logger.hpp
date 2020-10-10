#pragma once

#include <common/inttypes.hpp>
#include <interrupts/RegisterState.hpp>
#include <setup/HandOverState.hpp>

struct BootInfo;

class Logger {
public:
    void init(const hos_v1::FramebufferInfo &handOver);
    void flush();
    void setKernelColor();
    void setProgramNameColor();
    void setProgramColor();
    void basicWrite(char character);
    void output(char character);
    void sendCoreDump(size_t nameLength, const uint8_t *name, size_t dataLength, const uint8_t *data);
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

