#include <iostream>
#include <array>
#include <sys/mman.h>
#include <zagtos/Messaging.hpp>
#include "Port.hpp"
#include "Command.hpp"

void Port::ensureNotRunning() {
    std::cout << "Port is not idle. Attemting to change." << std::endl;
    regs.CMD.ST(0);
    while (regs.CMD.CR()) {}

    regs.CMD.FRE(0);
    while (regs.CMD.FR()) {}
}

void Port::waitWhileBusy() {
    while (regs.TFD.STS_BSY() || regs.TFD.STS_DRQ()) {}
}

size_t Port::allocateCommandSlot() {
    for (size_t slotIndex = 0; slotIndex < NumCommandSlots; slotIndex++) {
        if (!slotInUse[slotIndex]) {
            slotInUse[slotIndex] = true;
            return slotIndex;
        }
    }
    throw std::logic_error("command slot allocation while none where available");
}

void Port::freeCommandSlot(size_t slotIndex) {
    assert(slotInUse[slotIndex]);
    slotInUse[slotIndex] = false;
}

void Port::executeCommand(Command &command) {
    assert(&command.port == this);
    std::cout << "A" << std::endl;
    waitWhileBusy();
    std::cout << "B" << std::endl;
    regs.CI(1u << command.slotID);

    /*volatile uint32_t dummy = regs.CMD();
    dummy = dummy;*/
    std::cout << "C" << std::endl;

    while (regs.CI() & (1u << command.slotID)) {}
    std::cout << "D" << std::endl;

    if (!(regs.TFD.ERR() == 0 && regs.TFD.STS_ERR() == 0)) {
        throw std::logic_error("TODO: identify error");
    }
    std::cout << "E" << std::endl;
}

void Port::detectDevice() {
    static const uint32_t DET_PRESENT = 3, IPM_ACTIVE = 1, SIG_SATA = 0x00000101;
    if (regs.SSTS.DET() == DET_PRESENT && regs.SSTS.IPM() == IPM_ACTIVE && regs.SIG() == SIG_SATA) {
        if (!devicePresent) {
            std::cout << "detected SATA device" << std::endl;
            devicePresent = true;

            Command cmd(*this, ATACommand::IDENTIFY_DEVICE, 512, false);
            executeCommand(cmd);
        }
    } else {
        if (devicePresent) {
            std::cout << "lost SATA device" << std::endl;
            devicePresent = false;
        }
    }
}

void Port::enableInterrupts() {
    regs.IE.DHRE(1);
    regs.IE.UFE(1);
    regs.IE.PCE(1);
}

Port::Port(PortRegisters &regs):
    regs{regs},
    devicePresent{false} {

    ensureNotRunning();

    auto [shm, deviceAddresses] = zagtos::SharedMemory::DMA(MaximumDMAAddress, 0x500);
    assert(deviceAddresses.size() == 1);

    size_t clbAddr = deviceAddresses[0];
    size_t fbAddr = clbAddr + 0x400;
    regs.CLB(clbAddr);
    regs.CLBU(clbAddr >> 32);
    regs.FB(fbAddr);
    regs.FBU(fbAddr >> 32);

    regs.CMD.FRE(1);
    regs.SERR.ERR(0xffff);

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

    regs.IS(0);
}
