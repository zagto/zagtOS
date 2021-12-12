#include <iostream>
#include <tuple>
#include <zagtos/Messaging.hpp>
#include <zagtos/protocols/PS2.hpp>
#include <zagtos/protocols/Controller.hpp>

using namespace zagtos;

int main() {
    auto envPort = decodeRunMessage<RemotePort>(ps2::MSG_START_CONTROLLER);

    std::cout << "Starting PS/2 Controller..." << std::endl;
}
