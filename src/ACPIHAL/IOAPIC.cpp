#include <IOAPIC.hpp>
#include <iostream>
#include <zagtos/Messaging.hpp>
#include <zagtos/protocols/PS2.hpp>
#include <zagtos/protocols/Hal.hpp>
#include <vector>
extern "C" {
    #include <acpi.h>
}

using namespace zagtos;
using namespace zagtos::ioApic;


void initIoApic(zagtos::RemotePort &) {
    using namespace zagtos;

}
