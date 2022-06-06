/* for nanosleep */
#define _GNU_SOURCE 1
#include <iostream>
#include <memory>
#include <ctime>
#include <algorithm>
#include <sys/mman.h>
#include <zagtos/protocols/Driver.hpp>
#include <zagtos/Messaging.hpp>
#include <zagtos/protocols/Pci.hpp>
#include <zagtos/Register.hpp>
#include <zagtos/Interrupt.hpp>
#include "Registers.hpp"
#include "Controller.hpp"

using namespace zagtos;


int main() {
    std::cout << "Hello from AHCI" << std::endl;
    auto [controllerID, environemntPort, tuple] =
          decodeRunMessage<std::tuple<zagtos::UUID, RemotePort, std::tuple<RemotePort, pci::Device>>>(driver::MSG_START);
    assert(controllerID == driver::CONTROLLER_TYPE_PCI);
    auto [controllerPort, dev] = std::move(tuple);
    std::cout << "Decoded" << std::endl;

    if (!dev.BAR[5]) {
        throw std::runtime_error("PCI device claims to be AHCI device but does not implement BAR 5");
    }
    SharedMemory &ABARMemory = *dev.BAR[5];
    ABAR *abar = ABARMemory.map<ABAR>(PROT_READ|PROT_WRITE);
    assert(abar != nullptr);

    std::cout << "Mapped ABAR" << std::endl;

    zagtos::Port irqSetupPort;
    controllerPort.sendMessage(pci::MSG_ALLOCATE_MSI_IRQ, zbon::encode(irqSetupPort));
    auto interrupt = irqSetupPort.receiveMessage<Interrupt>(pci::MSG_ALLOCATE_MSI_IRQ_RESULT);
    interrupt.subscribe();

    Controller controller(*abar);

    std::cout << "Controller initialization OK" << std::endl;

    while (true) {
        interrupt.wait();
        std::cout << "Got AHCI interrupt" << std::endl;
        interrupt.processed();
    }
}
