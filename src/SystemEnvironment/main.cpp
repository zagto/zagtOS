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

UUID_DEFINE(MSG_BE_INIT, 0x72, 0x75, 0xb0, 0x4d, 0xdf, 0xc1, 0x41, 0x18,
            0xba, 0xbd, 0x0b, 0xf3, 0xfb, 0x79, 0x8e, 0x55);

int main() {
    std::cout << "Recieving Message..." << std::endl;

    receiveRunMessage(MSG_BE_INIT);

    std::cout << "Starting HAL..." << std::endl;

    Port port;
    std::cout << "num of handles in a port: " << zbon::encode(port.handle()).numHandles() << std::endl;
    environmentSpawn(ACPIHAL, Priority::BACKGROUND, StartHALMessage, zbon::encode(port.handle()));
    //environmentSpawn(ACPIHAL, Priority::BACKGROUND, {port.selfTag()}, StartHALMessage, zbon::encode(port.selfTag()));


    bool response;
    port.receiveMessage(StartHALResponse, response);
    std::cout << "HAL Start result: " << response << std::endl;
}
