#include <iostream>
#include <tuple>
#include <zagtos/Messaging.hpp>
#include <zagtos/protocols/PS2.hpp>
#include <zagtos/Interrupt.hpp>
#include <zagtos/protocols/Controller.hpp>

using namespace zagtos;

int main() {
    std::cout << "Starting PS/2 Controller..." << std::endl;

    using MsgType = std::tuple<RemotePort, std::vector<Interrupt>>;
    auto [envPort, interrupts] = decodeRunMessage<MsgType>(controller::MSG_START);

    std::cout << "got message" << std::endl;
}
