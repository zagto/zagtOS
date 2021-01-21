#include <iostream>
#include <array>
#include <zagtos/Messaging.hpp>
#include "Port.hpp"

void Port::ensureNotRunning() {
    std::cout << "Port is not idle. Attemting to change." << std::endl;
    regs.CMD.ST(0);
    while (regs.CMD.CR()) {}

    regs.CMD.FRE(0);
    while (regs.CMD.FR()) {}
}

void Port::detectDevice() {
    static const uint32_t DET_PRESENT = 3, IPM_ACTIVE = 1, SIG_SATA = 0x00000101;
    if (regs.SSTS.DET() == DET_PRESENT && regs.SSTS.IPM() == IPM_ACTIVE && regs.SIG() == SIG_SATA) {
        if (!devicePresent) {
            std::cout << "detected SATA device" << std::endl;
            devicePresent = true;
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

    regs.IS(0);
}
