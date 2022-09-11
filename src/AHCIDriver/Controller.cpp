#define _GNU_SOURCE 1 /* for nanosleep */
#include <iostream>
#include <limits>
#include <chrono>
#include <thread>
#include <zagtos/Messaging.hpp>
#include <zagtos/protocols/Pci.hpp>
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

    std::this_thread::sleep_for(std::chrono::milliseconds(25));

    while (regs.BOHC.BB()) {
        /* busy wait for BB to clear */
    }
    std::cout << "BIOS Handoff finished" << std::endl;
}

void Controller::reset() {
    /* backup capability registers */
    int32_t capBackup = regs.CAP();
    int32_t piBackup = regs.PI();

    /* enable AHCI */
    regs.GHC.AE(1);

    /* reset controller */
    regs.GHC.HR(1);
    while (regs.GHC.HR()) {
        /* busy wait for HR to clear */
    }
    std::cout << "did HR" << std::endl;
    regs.GHC.AE(1);
    std::cout << "did AE" << std::endl;

    regs.CAP(capBackup);
    regs.PI(piBackup);

    if (type == ControllerType::INTEL) {
        /* Intel controllers need an additional register in the PCI configuration space set to
         * work. Based on:
         * https://github.com/haiku/haiku/blob/17569c02ff5fdcb1feea243be687fc83568e206a/src/add-ons/kernel/busses/scsi/ahci/ahci_controller.cpp#L338 */
        std::cout << "Intel controller, setting PCS register" << std::endl;

        uint32_t numPorts = std::max<uint32_t>(
                    __builtin_popcount(regs.PI()),
                    regs.CAP.NP());
        if (numPorts > 8) {
            std::cout << "Applying Intel fix, but it does not support more than 8 AHCI ports and "
                      << "this controller has " << numPorts << std::endl;
            numPorts = 8;
        }

        static constexpr uint32_t PCS_REGISTER_INDEX = 36;
        zagtos::Port responsePort;
        pciControllerPort.sendMessage(
                    zagtos::pci::MSG_READ_CONFIG_SPACE,
                    zbon::encodeObject(responsePort,
                                       PCS_REGISTER_INDEX));
        auto readResult = responsePort.waitForMessage<std::optional<uint32_t>>(
                    zagtos::pci::MSG_READ_CONFIG_SPACE_RESULT);
        if (!readResult) {
            throw std::runtime_error("unable to read Intel PCS register");
        }

        uint32_t pcsRegister = *readResult | (((1u << numPorts) - 1) << 16);

        pciControllerPort.sendMessage(
                    zagtos::pci::MSG_WRITE_CONFIG_SPACE,
                    zbon::encodeObject(responsePort,
                                       PCS_REGISTER_INDEX,
                                       pcsRegister));
        bool writeResult = responsePort.waitForMessage<bool>(
                    zagtos::pci::MSG_WRITE_CONFIG_SPACE_RESULT);
        if (!writeResult) {
            throw std::runtime_error("unable to write Intel PCS register");
        }
    }
}

Controller::Controller(ABAR &abar, ControllerType type, zagtos::RemotePort &pciControllerPort) :
    type{type},
    pciControllerPort{pciControllerPort},
    regs{abar.controller} {

    /* enable AHCI */
    regs.GHC.AE(1);

    BIOSHandoff();

    reset();

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
    /* wait for devices to be detected */
    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    for (Port &port: ports) {
        port.detectDevice();
    }

    /* Setup interrupts */
    for (Port &port: ports) {
        port.enableInterrupts1();
    }
    regs.IS(0xffffffff);
    for (Port &port: ports) {
        port.enableInterrupts2();
    }
    regs.GHC.IE(1);
}
