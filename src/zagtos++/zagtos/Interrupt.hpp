#pragma once

#include <zagtos/HandleObject.hpp>

namespace zagtos {

enum class TriggerMode : size_t {
    LEVEL_LOW, LEVEL_HIGH, FALLING_EDGE, RISING_EDGE
};

struct ProcessorInterruptInfo {
    size_t processorID;
    size_t vectorNumber;
};

class Interrupt : public HandleObject {
private:
    static constexpr size_t CREATE_FIXED = 1;
    static constexpr size_t CREATE_PROCESSOR_DIRECT = 2;

public:
    Interrupt() {}
    Interrupt(uint32_t fixedNumber, TriggerMode triggerMode);
    Interrupt(TriggerMode triggerMode, ProcessorInterruptInfo &processorInterruptInfo);
    Interrupt(Interrupt &) = delete;
    Interrupt(Interrupt &&other) : HandleObject(std::move(other)) {}
    Interrupt &operator=(Interrupt &other) = delete;
    Interrupt &operator=(Interrupt &&other);

    void subscribe();
    void unsubscribe();
    bool wait();
    void processed();
};

}
