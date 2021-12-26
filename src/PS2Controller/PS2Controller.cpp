#include <iostream>
#include <tuple>
#include <zagtos/Messaging.hpp>
#include <zagtos/protocols/PS2.hpp>
#include <zagtos/Interrupt.hpp>
#include <zagtos/IOPortRange.hpp>
#include <zagtos/protocols/Controller.hpp>
#include <mutex>
#include <thread>
#include <chrono>
#include "Controller.hpp"


int main() {
    using MsgType = std::tuple<zagtos::RemotePort,
                               std::tuple<zagtos::Interrupt,
                                          zagtos::Interrupt,
                                          zagtos::IOPortRange>>;
    auto [envPort, tuple] = zagtos::decodeRunMessage<MsgType>(zagtos::controller::MSG_START);

    std::cout << "Hello" << std::endl;

    ps2controller::Controller controller(envPort,
                                         std::move(std::get<0>(tuple)),
                                         std::move(std::get<1>(tuple)),
                                         std::move(std::get<2>(tuple)));

    if (!controller.ports[0].works && !controller.ports[1].works) {
        throw std::runtime_error("No working ports!");
    }

    std::array<std::thread, 2> threads;

    for (size_t portIndex = 0; portIndex < 2; portIndex++) {
        if (controller.ports[portIndex].works) {
            std::cout << "Port " << portIndex << " works" << std::endl;
            threads[portIndex] = std::thread(&ps2controller::Port::handler,
                                             &controller.ports[portIndex]);
        }
    }

    for (size_t portIndex = 0; portIndex < 2; portIndex++) {
        if (controller.ports[portIndex].works) {
            threads[portIndex].join();
        }
    }
}
