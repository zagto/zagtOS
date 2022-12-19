#include <iostream>
#include <tuple>
#include <zagtos/Messaging.hpp>
#include <zagtos/protocols/PS2.hpp>
#include <zagtos/Interrupt.hpp>
#include <zagtos/IOPortRange.hpp>
#include <zagtos/protocols/Driver.hpp>
#include <zagtos/Topology.hpp>
#include <chrono>
#include "Controller.hpp"

/* Pin driver to default Processor. This ensures no interrupts are processed out-of-order on other
 * Processors. More importantly, this works arount VirtualBox brokenness when moving the PS/2
 * controller driver between Processors. */
zagtos::Processor driverProcessor;

void handlerThreadEntry(ps2controller::Port &port) {
    driverProcessor.pinCurrentThread();
    port.handler();
}

int main() {
    exit(0);
    using MsgType = std::tuple<zagtos::UUID,
                               zagtos::RemotePort,
                               std::tuple<zagtos::Interrupt,
                                          zagtos::Interrupt,
                                          zagtos::IOPortRange>>;
    auto [controllerID, envPort, tuple] = zagtos::decodeRunMessage<MsgType>(zagtos::driver::MSG_START);

    std::cout << "Hello" << std::endl;

    driverProcessor.pinCurrentThread();

    ps2controller::Controller controller(envPort,
                                         std::move(std::get<0>(tuple)),
                                         std::move(std::get<1>(tuple)),
                                         std::move(std::get<2>(tuple)));

    if (!controller.ports[0].works && !controller.ports[1].works) {
        throw std::runtime_error("No working ports!");
    }


}
