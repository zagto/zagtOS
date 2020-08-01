#include <iostream>
#include <memory>
#include <sys/mman.h>
#include <zagtos/Controller.hpp>
#include <zagtos/Messaging.hpp>
#include <zagtos/PCI.hpp>

using namespace zagtos;
using namespace zagtos::pci;


int main() {
    pci::Device dev = decodeRunMessage<pci::Device>(MSG_START_PCI_DRIVER);
}
