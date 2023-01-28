#include <cstddef>
#include <zagtos/Align.hpp>
#include "Device.hpp"
#include "Command.hpp"

Command::Command(Device &device):
    device{device},
    slotID{device.port.allocateCommandSlot()},
    header{device.port.commandList[slotID]},
    table{device.port.commandTables[slotID]} {

    /* clear all fields the driver uses besides the command table pointers - they are constant in
     * this driver. */
    memset(&header,
           0,
           reinterpret_cast<size_t>(&header.CTBA0) - reinterpret_cast<size_t>(&header));
}

Command::~Command() {
    device.port.freeCommandSlot(slotID);
}

Command::Command(ATACommand cmd,
                 uint64_t startSector,
                 size_t startPage,
                 size_t numSectors,
                 bool write,
                 MemoryArea *_memoryArea) :
        Command(_memoryArea->device) {
    memoryArea = _memoryArea;

    /* TODO: check max length & 2-byte alignment */
    assert(PAGE_SIZE % device.sectorSize == 0);
    size_t sectorsPerPage = PAGE_SIZE / device.sectorSize;
    size_t numPages = (numSectors + sectorsPerPage - 1) / sectorsPerPage;
    std::cout << "deviceAddresses.size: " << memoryArea->deviceAddresses.size() << std::endl;
    std::cout << "startPage: " << startPage << std::endl;
    std::cout << "numPages: " << numPages << std::endl;
    assert(memoryArea->deviceAddresses.size() >= startPage + numPages);

    table.h2d = H2DFIS();
    table.h2d.command(cmd);
    table.h2d.lba(startSector);
    table.h2d.sectorCount(numSectors);

    for (size_t pageIndex = 0; pageIndex < numPages; pageIndex++) {
        size_t address = memoryArea->deviceAddresses[startPage + pageIndex];
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

    header.W = write;
    header.PRDTL = numPages;
}
