#pragma once

#include <vector>
#include "Registers.hpp"
#include "Port.hpp"

class Controller {
private:
    ControllerRegisters &regs;
    std::vector<Port> ports;
    bool supports64Bit;

    void BIOSHandoff();

public:
    Controller(ABAR &);
    void handleInterrupt();
};
