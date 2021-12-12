#pragma once

#include <zagtos/Messaging.hpp>

namespace zagtos {

enum class TriggerMode : size_t {
    Edge, Level
};
enum class Polarity : size_t {
    ActiveHigh, ActiveLow
};

class Interrupt : public HandleObject {
private:
    static constexpr size_t CREATE_FIXED = 1;
    static constexpr size_t CREATE_ANY = 2;

public:
    Interrupt() {}
    Interrupt(uint32_t fixedNumber, TriggerMode triggerMode, Polarity polarity);
    Interrupt(Interrupt &) = delete;
    Interrupt(Interrupt &&other) : HandleObject(std::move(other)) {}
    Interrupt &operator=(Interrupt &&other);

    void subscribe();
    void unsubscribe();
    bool wait();
    void processed();
};

}
