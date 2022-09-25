#pragma once

#include <optional>
#include <memory>
#include "Registers.hpp"
#include "PortStructures.hpp"

class Command;
class Device;

class Port {
private:
    PortRegisters &regs;
    DSFISClass *dsfis;
    PSFISClass *psfis;
    RFISClass *rfis;
    UFISClass *ufis;
    std::array<bool, MAX_NUM_COMMAND_SLOTS> slotInUse{false};

    void ensureNotRunning();
    void waitWhileBusy();
    void executeCommand(Command &command);
    void reset();

protected:
    friend class Command;
    CommandHeader *commandList;
    CommandTable *commandTables;

    size_t allocateCommandSlot();
    void freeCommandSlot(size_t);

public:
    std::unique_ptr<Device> device;

    /* called after initizalization by the contoller it cleared IS.IPS. This sequence is
     * recommended by section 10.1.2 System Software Specific Initialization in the AHCI spec. */
    void enableInterrupts1();
    void enableInterrupts2();
    void detectDevice();

    Port(PortRegisters &regs);
};
