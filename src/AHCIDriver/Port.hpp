#pragma once

#include <memory>
#include <queue>
#include "Registers.hpp"
#include "PortStructures.hpp"
#include "PhysicalCommand.hpp"
#include "LogicalCommand.hpp"

class Device;

class Port {
private:
    const size_t id;
    PortRegisters &regs;
    DSFISClass *dsfis;
    PSFISClass *psfis;
    RFISClass *rfis;
    UFISClass *ufis;
    std::array<std::unique_ptr<PhysicalCommand>, MAX_NUM_COMMAND_SLOTS> slotCommands;
    std::queue<std::unique_ptr<LogicalCommand>> commandQueue;
    size_t numFreeSlots;

    void ensureNotRunning();
    void waitWhileBusy();
    void reset();

protected:
    friend class PhysicalCommand;
    CommandHeader *commandList;
    CommandTable *commandTables;

    size_t allocateCommandSlot();
    void freeCommandSlot(size_t);
    bool commandSlotAvailable() const;

public:
    std::unique_ptr<Device> device;

    /* called after initizalization by the contoller it cleared IS.IPS. This sequence is
     * recommended by section 10.1.2 System Software Specific Initialization in the AHCI spec. */
    void enableInterrupts1();
    void enableInterrupts2();
    void detectDevice();
    void submitLogicalCommand(std::unique_ptr<LogicalCommand> logicalCommand);
    void submitPhysicalCommand(std::unique_ptr<PhysicalCommand> command, bool syncronous);
    void checkCommandsComplete();

    Port(PortRegisters &regs, size_t id);
};
