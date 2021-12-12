/* for nanosleep */
#define _GNU_SOURCE 1
#include <iostream>
#include <memory>
#include <ctime>
#include <algorithm>
#include <sys/mman.h>
#include <zagtos/protocols/Controller.hpp>
#include <zagtos/Messaging.hpp>
#include <zagtos/protocols/Pci.hpp>
#include <zagtos/Register.hpp>
#include "Registers.hpp"
#include "Controller.hpp"

using namespace zagtos;


int main() {
    auto [controllerPort, dev] =
            decodeRunMessage<std::tuple<RemotePort, pci::Device>>(pci::MSG_START_PCI_DRIVER);
    std::cout << "Hello from AHCI" << std::endl;

    if (!dev.BAR[5]) {
        throw std::runtime_error("PCI device claims to be AHCI device but does not implement BAR 5");
    }
    SharedMemory &ABARMemory = *dev.BAR[5];
    ABAR *abar = ABARMemory.map<ABAR>(PROT_READ|PROT_WRITE);
    assert(abar != nullptr);

    std::cout << "Mapped ABAR" << std::endl;

    zagtos::Port irqSetupPort;
    controllerPort.sendMessage(pci::MSG_ALLOCATE_MSI_IRQ, zbon::encode(irqSetupPort));

    Controller controller(*abar);

    std::cout << "Controller initialization OK" << std::endl;
}
