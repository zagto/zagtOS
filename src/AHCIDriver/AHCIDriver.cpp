#include <iostream>
#include <memory>
#include <sys/mman.h>
#include <zagtos/Controller.hpp>
#include <zagtos/Messaging.hpp>
#include <zagtos/PCI.hpp>

using namespace zagtos;
using namespace zagtos::pci;

static volatile struct ABARStruct {
    const uint32_t HBACapabilities;
} *ABAR;

int main() {
    pci::Device dev = decodeRunMessage<pci::Device>(MSG_START_PCI_DRIVER);
    std::cout << "Hello from AHCI" << std::endl;

    if (!dev.BAR[5]) {
        throw std::runtime_error("device claims to be AHCI device but does not implement BAR 5");
    }
    pci::BaseRegister &ABARDesc = *dev.BAR[5];
    ABAR = static_cast<ABARStruct *>(ABARDesc.sharedMemory.map(0, ABARDesc.length, PROT_READ|PROT_WRITE));
    assert(ABAR != nullptr);

    std::cout << "Mapped ABAR" << std::endl;
}
