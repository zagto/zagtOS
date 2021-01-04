#pragma once

#include "Registers.hpp"
#include "FIS.hpp"
#include "CommandList.hpp"

class Port {
private:
    PortRegisters &regs;
    FIS *DSFIS;
    FIS *PSFIS;
    FIS *RFIS;
    bool devicePresent;

    void ensureNotRunning();
    void detectDevice();

public:
    /* called after initizalization by the contoller it cleared IS.IPS. This sequence is
     * recommended by section 10.1.2 System Software Specific Initialization in the AHCI spec. */
    void enableInterrupts();

    Port(PortRegisters &regs);
};
