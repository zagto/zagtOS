#include <cstddef>
#include <zagtos/Align.hpp>
#include "Command.hpp"

Command::Command(Port &port):
    port{port},
    slotID{port.allocateCommandSlot()},
    header{port.commandList[slotID]},
    table{port.commandTables[slotID]} {

    memset(&header, 0, sizeof(header));
}

Command::~Command() {
    port.freeCommandSlot(slotID);
}

Command::Command(Port &port, ATACommand cmd, size_t length, bool write) : Command(port) {
    size_t numPages = zagtos::align(length, PAGE_SIZE, zagtos::AlignDirection::UP) / PAGE_SIZE;

    /* TODO: check max length & 2-byte alignment */

    std::vector<size_t> deviceAddresses;
    std::tie(dataMemory, deviceAddresses) = zagtos::SharedMemory::DMA(MaximumDMAAddress, length);
    assert(deviceAddresses.size() == numPages);

    table.h2d = H2DFIS();
    table.h2d.command(cmd);

    for (size_t pageIndex = 0; pageIndex < numPages; pageIndex++) {
        size_t address = deviceAddresses[pageIndex];
        table.PRDT[pageIndex] = {
            .DBA = static_cast<uint32_t>(address),
            .DBAU = static_cast<uint32_t>(address >> 32),
            .reserved0 = 0,
            .DBC = PAGE_SIZE - 1,
            .reserved1 = 0,
            .I = 0,
        };
    }

    header.CFL = sizeof(H2DFIS) / 4;
    header.W = write;
    header.PRDTL = numPages;

}
