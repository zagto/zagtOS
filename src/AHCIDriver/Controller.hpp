#pragma once

#include <vector>
#include "Registers.hpp"
#include "Port.hpp"
#include "zagtos/Messaging.hpp"

namespace zagtos {
class RemotePort;
}

enum class ControllerType {
    STANDARD, INTEL
};

class Controller {
private:
    const ControllerType type;
    zagtos::RemotePort &pciControllerPort;
    ControllerRegisters &regs;
    std::vector<Port> ports;
    bool supports64Bit;

    void BIOSHandoff();
    void reset();

public:
    Controller(ABAR &, ControllerType type, zagtos::RemotePort &pciControllerPort);
    void handleInterrupt();
    std::vector<const Device *> allDevices() const;
};
