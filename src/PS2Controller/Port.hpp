#pragma once

#include <zagtos/Interrupt.hpp>

namespace ps2controller {

class Controller;

class Port {
private:

    Controller &controller;
    size_t portIndex;

public:
    zagtos::Interrupt interrupt;
    bool works = false;

    Port(Controller &controller, zagtos::Interrupt interrupt, size_t portIndex);

    void sendToDevice(uint8_t value);
    void handler();
};

}
