#include <iostream>
#include <array>
#include <sys/mman.h>
#include <zagtos/Messaging.hpp>
#include <chrono>
#include <thread>
#include "Port.hpp"
#include "PhysicalCommand.hpp"
#include "LogicalCommand.hpp"
#include "Device.hpp"
#include "MemoryArea.hpp"


void Port::ensureNotRunning() {
    std::cout << "ensureNotRunning" << std::endl;
    if (regs.CMD.ST()) {
        regs.CMD.ST(0);
        while (regs.CMD.CR()) {}
    }

    if (regs.CMD.FRE()) {
        regs.CMD.FRE(0);
        while (regs.CMD.FR()) {}
    }

    std::cout << "END ensureNotRunning" << std::endl;
}

void Port::waitWhileBusy() {
    while (regs.TFD.STS_BSY() || regs.TFD.STS_DRQ()) {
        std::cout << "waiting for STS_BSY ("<<regs.TFD.STS_BSY()<<") and STS_DRQ ("<<regs.TFD.STS_DRQ()<<") clear" << std::endl;

    }
}

size_t Port::allocateCommandSlot() {
    assert(numFreeSlots > 0);
    for (size_t slotIndex = 0; slotIndex < NumCommandSlots; slotIndex++) {
        if (!slotCommands[slotIndex]) {
            numFreeSlots--;
            return slotIndex;
        }
    }
    throw std::logic_error("numFreeSlots and used slots out of sync");
}

void Port::freeCommandSlot(size_t slotIndex) {
    numFreeSlots++;
    assert(!slotCommands[slotIndex]);
}

bool Port::commandSlotAvailable() const {
    return numFreeSlots > 0;
}

void Port::submitLogicalCommand(std::unique_ptr<LogicalCommand> logicalCommand) {
    if (commandSlotAvailable()) {
        assert(commandQueue.empty());
        const size_t slotID = allocateCommandSlot();
        auto physicalCommand = std::make_unique<PhysicalCommand>(std::move(logicalCommand), slotID);
        submitPhysicalCommand(std::move(physicalCommand), false);
    } else {
        commandQueue.push(std::move(logicalCommand));
    }
}

void Port::submitPhysicalCommand(std::unique_ptr<PhysicalCommand> command, bool syncronous) {
    assert(&command->device.port == this);
    const size_t slotID = command->slotID;
    assert(!slotCommands[slotID]);
    slotCommands[slotID] = std::move(command);
    waitWhileBusy();
    regs.CI(1u << slotID);

    if (syncronous) {
        while (regs.CI() & (1u << slotID)) {}
        slotCommands[slotID] = {};
        if (!(regs.TFD.ERR() == 0 && regs.TFD.STS_ERR() == 0)) {
            std::cout << "Failed to execute command: TFD.ERR: " << regs.TFD.ERR()
                      << ", TFD.STS.ERR: " << regs.TFD.STS_ERR() << std::endl;
            throw std::logic_error("TODO: command execution error error"); // COMRESET?
        }
    }
}

void Port::checkCommandsComplete() {
    for (size_t slotID = 0; slotID < NumCommandSlots; slotID++) {
        if (slotCommands[slotID]) {
            /* check if command issue register got cleared */
            if ((regs.CI() & (1u << slotID)) == 0) {
                if (!(regs.TFD.ERR() == 0 && regs.TFD.STS_ERR() == 0)) {
                    std::cout << "Failed to execute command: TFD.ERR: " << regs.TFD.ERR()
                              << ", TFD.STS.ERR: " << regs.TFD.STS_ERR() << std::endl;
                    throw std::logic_error("TODO: command execution error error"); // COMRESET?
                }

                /* success */
                std::unique_ptr<LogicalCommand> logicalCommand =
                        std::move(slotCommands[slotID]->logicalCommand);
                slotCommands[slotID] = {};
                logicalCommand->memoryArea->commandComplete(*logicalCommand);
                logicalCommand = {};

                if (commandQueue.empty()) {
                    freeCommandSlot(slotID);
                } else {
                    logicalCommand = std::move(commandQueue.front());
                    commandQueue.pop();
                    auto physicalCommand =
                            std::make_unique<PhysicalCommand>(std::move(logicalCommand), slotID);
                    submitPhysicalCommand(std::move(physicalCommand), false);
                }
            }
        }
    }
}

void Port::detectDevice() {
    /* clear errors from port reset */
    regs.SERR(0xffffffff);

    /* DET can have these values:
     * 0 - no device
     * 1 - device present but no connection
     * 3 - connection established
     * 4 - offline */
    static const uint32_t DET_PRESENT = 3, IPM_ACTIVE = 1, SIG_SATA = 0x00000101;

    std::cout << "Port DET: " << regs.SSTS.DET() << " IPM: " << regs.SSTS.IPM() << std::endl;
    if (regs.SSTS.DET() == DET_PRESENT && regs.SSTS.IPM() == IPM_ACTIVE) {
        regs.CMD.FRE(1);
        while (!regs.CMD.FRE()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            std::cout << "waiting for regs.CMD.FRE to be set" << std::endl;
        }
        waitWhileBusy();
        regs.CMD.ST(1);

        std::cout << "Port SIG:" <<regs.SIG() << std::endl;

        if (regs.SIG() == SIG_SATA) {

            if (!device) {
                std::cout << "detected SATA device" << std::endl;

                Device temporaryDevice(512, 1, *this);
                MemoryArea memoryArea(temporaryDevice, PAGE_SIZE);
                auto logicalCommand = std::make_unique<LogicalCommand>(LogicalCommand{
                    .memoryArea = &memoryArea,
                    .action = LogicalCommand::Action::IDENTIFY,
                    .cookie = 0,
                    .startSector = 0,
                    .startPage = 0,
                    .numSectors = 1,
                    .responsePort = {},
                });
                const size_t slotID = allocateCommandSlot();
                auto physicalCommand =
                        std::make_unique<PhysicalCommand>(std::move(logicalCommand), slotID);
                submitPhysicalCommand(std::move(physicalCommand), true);
                freeCommandSlot(slotID);

                auto identify = memoryArea.sharedMemory.map<IdentifyDeviceData>(PROT_READ);

                uint64_t sectorSize = static_cast<uint64_t>(identify->wordsPerLogicalSector) * 2;
                if (sectorSize == 0) {
                    /* use default sector size if wordsPerLogicalSector property is not set */
                    sectorSize = 512;
                }

                uint64_t numSectors = 0;
                if ((identify->supportedCommandSets & SupportedCommandSet::LBA48) && identify->maxLBA48) {
                    std::cout << "device has " << identify->maxLBA48 << " LBA48 sectors of size " << sectorSize << std::endl;
                    numSectors = identify->maxLBA48;
                } else if (identify->maxLBA28 != 0) {
                    std::cout << "device has " << identify->maxLBA28 << " LBA28 sectors of size " << sectorSize << std::endl;
                    numSectors = identify->maxLBA28;
                }

                zagtos::UnmapWhole(identify);

                if (numSectors == 0) {
                    std::cout << "Ignoring device that does not support LBA addressing"
                              << std::endl;
                    return;
                }
                device = std::make_unique<Device>(sectorSize, numSectors, *this);
            }
        }
    } else {
        if (device) {
            std::cout << "lost SATA device" << std::endl;
            device = {};
        }
    }
}

void Port::reset() {
    /* Port reset */
    regs.SCTL.DET(1);
    std::this_thread::sleep_for(std::chrono::milliseconds(2));

    regs.SCTL.DET(0);
}

void Port::enableInterrupts1() {
    /* clear port IS, before we can clear controller IS.IPS, before we can program IE
     *  - 10.1.2 System Software Specific Initialization */
    regs.IS(0xffffffff);
}

void Port::enableInterrupts2() {
    regs.IE.DHRE(1);
    regs.IE.UFE(1);
    regs.IE.PCE(1);
}

Port::Port(PortRegisters &regs, size_t id) :
    id{id},
    regs{regs} {

    ensureNotRunning();

    auto [shm, deviceAddresses] = zagtos::SharedMemory::DMA(MaximumDMAAddress, 0x500);
    assert(deviceAddresses.size() == 1);

    size_t clbAddr = deviceAddresses[0];
    size_t fbAddr = clbAddr + 0x400;
    regs.CLB(clbAddr);
    regs.CLBU(clbAddr >> 32);
    regs.FB(fbAddr);
    regs.FBU(fbAddr >> 32);

    /* Set port to non-idle state and start communication (not necessary?) */
    while (regs.CMD.ICC() != 0) {
        std::cout << "waiting for ICC to clear" << std::endl;
    }
    regs.CMD.POD(1);
    regs.CMD.SUD(1);
    regs.CMD.ICC(1);

    /* disable all power management states for now */
    regs.SCTL.IPM(7);
    /* no limit to connection speed */
    regs.SCTL.SPD(0);

    reset();

    commandList = shm.map<CommandHeader>(PROT_READ|PROT_WRITE);
    size_t fisPointer = reinterpret_cast<size_t>(commandList) + 0x400;
    dsfis = reinterpret_cast<DSFISClass *>(fisPointer);
    psfis = reinterpret_cast<PSFISClass *>(fisPointer + 0x20);
    rfis  = reinterpret_cast<RFISClass  *>(fisPointer + 0x40);
    ufis  = reinterpret_cast<UFISClass  *>(fisPointer + 0x60);

    auto [shm2, deviceAddresses2] = zagtos::SharedMemory::DMA(MaximumDMAAddress, PAGE_SIZE * NumCommandSlots);
    assert(deviceAddresses2.size() == NumCommandSlots);
    commandTables = shm2.map<CommandTable>(PROT_READ|PROT_WRITE);

    for (size_t slotIndex = 0; slotIndex < NumCommandSlots; slotIndex++) {
        commandList[slotIndex].CTBA0 = static_cast<uint32_t>(deviceAddresses2[slotIndex]);
        commandList[slotIndex].CTBA_U0 = deviceAddresses2[slotIndex] >> 32;
    }
    numFreeSlots = NumCommandSlots;
}
