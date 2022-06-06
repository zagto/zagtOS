#define _GNU_SOURCE 1 /* for nanosleep */
#include <iostream>
#include <limits>
#include <ctime>
#include "Controller.hpp"

size_t MaximumDMAAddress;
size_t NumCommandSlots;

void Controller::BIOSHandoff() {
    if (!regs.CAP2.BOH()) {
        std::cout << "No BIOS Handoff supported" << std::endl;
        return;
    }

    regs.BOHC.OOS(1);
    while (regs.BOHC.BOS()) {
        /* busy wait for BOS to clear */
    }

    timespec ts{0, 25000};
    nanosleep(&ts, nullptr);

    while (regs.BOHC.BB()) {
        /* busy wait for BB to clear */
    }
    std::cout << "BIOS Handoff finished" << std::endl;
}

Controller::Controller(ABAR &abar) :
    regs{abar.controller} {

    /* enable AHCI */
    regs.GHC.AE(1);

    BIOSHandoff();

    /* reset controller */
    regs.GHC.HR(1);
    regs.GHC.AE(1);

    if (regs.CAP.S64A() && sizeof(size_t) > 4) {
        MaximumDMAAddress = std::numeric_limits<uint64_t>::max();
    } else {
        MaximumDMAAddress = std::numeric_limits<uint32_t>::max();
    }
    NumCommandSlots = regs.CAP.NCS();

    uint32_t implementedMask = regs.PI();
    for (size_t portID = 0; portID < 32; portID++) {
        if (implementedMask & (1 << portID)) {
            ports.emplace_back(abar.ports[portID]);
        }
    }

    for (Port &port: ports) {
        port.detectDevice();
    }

    /* Setup interrupts */
    regs.IS(0);
    for (Port &port: ports) {
        port.enableInterrupts();
    }
    regs.GHC.IE(1);
}
