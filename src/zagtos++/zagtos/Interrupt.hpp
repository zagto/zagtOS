#pragma once

#include <zagtos/HandleObject.hpp>

namespace zagtos {

enum class TriggerMode : size_t {
    LEVEL_LOW, LEVEL_HIGH, FALLING_EDGE, RISING_EDGE
};

class Interrupt : public HandleObject {
private:
    static constexpr size_t CREATE_FIXED = 1;
    static constexpr size_t CREATE_ANY = 2;

public:
    Interrupt() {}
    Interrupt(uint32_t fixedNumber, TriggerMode triggerMode);
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
