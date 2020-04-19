#include <iostream>
#include <thread>
#include <tuple>
#include <zagtos/ZBON.hpp>
#include <zagtos/Messaging.hpp>
#include <zagtos/HAL.hpp>
#include <zagtos/ExternalBinary.hpp>
#include <zagtos/EnvironmentSpawn.hpp>
#include <zagtos/Controller.hpp>

EXTERNAL_BINARY(ACPIHAL)
EXTERNAL_BINARY(PCIController)

using namespace zagtos;

UUID_DEFINE(MSG_BE_INIT, 0x72, 0x75, 0xb0, 0x4d, 0xdf, 0xc1, 0x41, 0x18,
            0xba, 0xbd, 0x0b, 0xf3, 0xfb, 0x79, 0x8e, 0x55);

void ControllerServer(const ExternalBinary &program, zbon::EncodedData startMessage) {
    std::cout << "SPAWNING PCI..." << std::endl;
    Port port;
    environmentSpawn(program,
                     Priority::BACKGROUND,
                     MSG_START_CONTROLLER,
                     zbon::encode(std::make_tuple(port.handle(), std::move(startMessage))));
}

int main() {
    std::cout << "Recieving Message..." << std::endl;

    receiveRunMessage(MSG_BE_INIT);

    std::cout << "Starting HAL..." << std::endl;

    Port halServerPort;
    environmentSpawn(ACPIHAL, Priority::BACKGROUND, MSG_START_HAL, zbon::encode(halServerPort.handle()));

    while (true) {
        std::unique_ptr<MessageInfo> msgInfo = halServerPort.receiveMessage();
        if (uuid_compare(msgInfo->type, zagtos::MSG_START_HAL_RESULT) == 0) {
            bool result;
            if (!zbon::decode(msgInfo->data, result)) {
                std::cout << "Received malformed MSG_START_HAL_RESULT message." << std::endl;
                continue;
            }

            if (result) {
                std::cout << "HAL startup complete. TODO: what's next?" << std::endl;
            } else {
                std::cout << "HAL startup failed." << std::endl;
            }
        } else if (uuid_compare(msgInfo->type, zagtos::MSG_FOUND_CONTROLLER) == 0) {
            std::tuple<uuid_t, zbon::EncodedData> msg;
            if (!zbon::decode(msgInfo->data, msg)) {
                std::cout << "Received malformed MSG_FOUND_CONTROLLER message." << std::endl;
                continue;
            }

            if (uuid_compare(std::get<0>(msg), CONTROLLER_TYPE_PCI) == 0) {
                std::cout << "SPAWNING PCI A..." << std::endl;

                new std::thread(ControllerServer, PCIController, std::move(std::get<1>(msg)));
            } else {
                std::cout << "Received MSG_START_CONTROLLER message for unsupported controller "
                          << "type." << std::endl;
                continue;
            }
        } else {
            std::cout << "Got message of unknown type" << std::endl;
        }
    }
}
