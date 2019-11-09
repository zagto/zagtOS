#include <iostream>
#include <fstream>
#include <tuple>
#include <zagtos/zbon.hpp>
#include <zagtos/Messaging.hpp>
#include <zagtos/HAL.hpp>
#include <zagtos/ExternalBinary.hpp>
#include <zagtos/EnvironmentSpawn.hpp>

EXTERNAL_BINARY(ACPIHAL)

using namespace zagtos;

int main() {
    std::cout << "Starting HAL..." << std::endl;

    Port port;
    //environmentSpawn(ACPIHAL, {port.selfTag()}, StartHALMessage, zbon::encode(std::make_tuple(port.selfTag())));
    environmentSpawn(ACPIHAL, Priority::BACKGROUND, {port.selfTag()}, StartHALMessage, zbon::encode(port.selfTag()));
    //StartHALResponse response = port.receiveMessage(msg);
}
