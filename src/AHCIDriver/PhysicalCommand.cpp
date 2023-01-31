#include <cstddef>
#include <zagtos/Align.hpp>
#include "Device.hpp"
#include "Port.hpp"
#include "PhysicalCommand.hpp"
#include "LogicalCommand.hpp"

PhysicalCommand::~PhysicalCommand() {
    device.port.freeCommandSlot(slotID);
}

PhysicalCommand::PhysicalCommand(std::unique_ptr<LogicalCommand> _logicalCommand, size_t slotID):
        logicalCommand(std::move(_logicalCommand)),
        device{logicalCommand->memoryArea->device},
        slotID{slotID},
        header{device.port.commandList[slotID]},
        table{device.port.commandTables[slotID]} {

    /* clear all fields the driver uses besides the command table pointers - they are constant in
     * this driver. */
    memset(&header,
           0,
           reinterpret_cast<size_t>(&header.CTBA0) - reinterpret_cast<size_t>(&header));

    memoryArea = logicalCommand->memoryArea;
    uint64_t numSectors = logicalCommand->numSectors;
    assert(numSectors >= 1);
    assert(memoryArea->device.sectorSize >= 1);
    size_t numPages = (numSectors * memoryArea->device.sectorSize - 1) / PAGE_SIZE + 1;
    assert(memoryArea->deviceAddresses.size() >= logicalCommand->startPage + numPages);

    table.h2d = H2DFIS();
    table.h2d.command(ataCommandFor(logicalCommand->action));
    table.h2d.lba(logicalCommand->startSector);
    table.h2d.sectorCount(numSectors);

    for (size_t pageIndex = 0; pageIndex < numPages; pageIndex++) {
        size_t address = memoryArea->deviceAddresses[logicalCommand->startPage + pageIndex];
        table.PRDT[pageIndex] = {
            .DBA = static_cast<uint32_t>(address),
            .DBAU = static_cast<uint32_t>(address >> 32),
            .reserved0 = 0,
            .DBC = PAGE_SIZE - 1,
            .reserved1 = 0,
            .I = 0,
        };
    }

    /* length of command FIS in DWORDS (without crc field?) */
    header.CFL = 5;

    header.W = actionIsWrite(logicalCommand->action);
    header.PRDTL = numPages;
}

ATACommand PhysicalCommand::ataCommandFor(LogicalCommand::Action action) {
    switch (action) {
    case LogicalCommand::Action::READ:
        return ATACommand::READ_DMA_EXT;
    case LogicalCommand::Action::WRITE:
        return ATACommand::WRITE_DMA_EXT;
    case LogicalCommand::Action::IDENTIFY:
        return ATACommand::IDENTIFY_DEVICE;
    }
    throw std::logic_error("missing logical Action case");
}

bool PhysicalCommand::actionIsWrite(LogicalCommand::Action action) {
    return action == LogicalCommand::Action::WRITE;
}
