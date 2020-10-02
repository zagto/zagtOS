#include <iostream>
#include <thread>
#include <tuple>
#include <vector>
#include <zagtos/ZBON.hpp>
#include <zagtos/Messaging.hpp>
#include <zagtos/HAL.hpp>
#include <zagtos/ExternalBinary.hpp>
#include <zagtos/EnvironmentSpawn.hpp>
#include <zagtos/Controller.hpp>

EXTERNAL_BINARY(ACPIHAL)
EXTERNAL_BINARY(PCIController)
EXTERNAL_BINARY(AHCIDriver)

using namespace zagtos;

UUID_DEFINE(MSG_BE_INIT, 0x72, 0x75, 0xb0, 0x4d, 0xdf, 0xc1, 0x41, 0x18,
            0xba, 0xbd, 0x0b, 0xf3, 0xfb, 0x79, 0x8e, 0x55);

struct DeviceMatch {
    uint64_t id;
    uint64_t idMask;
};

class Driver {
private:
    std::vector<DeviceMatch> _matches;
    const ExternalBinary &binary;

public:
    Driver(std::vector<DeviceMatch> matches, const ExternalBinary &binary):
        _matches{matches},
        binary{binary} {}

    bool matches(uint64_t deviceID) const {
        for (auto match: _matches) {
            if (match.id == (deviceID & match.idMask)) {
                return true;
            }
        }
        return false;
    }

    void start(const uuid_t messageType, zbon::EncodedData runMessage) const {
        environmentSpawn(binary, Priority::BACKGROUND, messageType, std::move(runMessage));
    }

    const char *name() const {
        return binary.logName();
    }
};

struct ControllerType {
    const uuid_t &id;
    const uuid_t &driverStartMessageID;
    std::vector<Driver> drivers;
};

static Driver dACHIDriver{{{0x0106'0000'0000'0000, 0xffff'0000'0000'0000}}, AHCIDriver};
static ControllerType PCI{CONTROLLER_TYPE_PCI, MSG_START_PCI_DRIVER, {dACHIDriver}};

void ControllerServer(const ExternalBinary &program,
                      zbon::EncodedData startMessage,
                      const ControllerType &controllerType) {
    std::cout << "SPAWNING PCI..." << std::endl;
    Port port;
    environmentSpawn(program,
                     Priority::BACKGROUND,
                     MSG_START_CONTROLLER,
                     zbon::encodeObject(port, std::move(startMessage)));

    while (true) {
        std::unique_ptr<MessageInfo> msgInfo = port.receiveMessage();
        if (uuid_compare(msgInfo->type, MSG_START_CONTROLLER_DONE) == 0) {
            std::cerr << "got MSG_START_CONTROLLER_DONE, TODO: what is next?" << std::endl;
        } else if (uuid_compare(msgInfo->type, MSG_FOUND_DEVICE) == 0) {
            std::tuple<uint64_t, zbon::EncodedData> msg;
            try {
                zbon::decode(msgInfo->data, msg);
            } catch(zbon::DecoderException &e) {
                std::cout << "Received malformed MSG_FOUND_DEVICE message." << std::endl;
                continue;
            }
            uint64_t deviceID = std::get<0>(msg);
            std::cout << "combined device id: " << deviceID << std::endl;
            zbon::EncodedData &startData = std::get<1>(msg);
            for (const Driver &driver: controllerType.drivers) {
                if (driver.matches(deviceID)) {
                    std::cout << "Matched driver " << driver.name() << std::endl;
                    driver.start(controllerType.driverStartMessageID, std::move(startData));
                    continue;
                }
            }
            std::cout << "No matching driver" << std::endl;
        }
    }
}

int main() {
    receiveRunMessage(MSG_BE_INIT);

    std::cout << "Starting HAL..." << std::endl;

    Port halServerPort;
    environmentSpawn(ACPIHAL, Priority::BACKGROUND, MSG_START_HAL, zbon::encode(halServerPort));

    while (true) {
        std::unique_ptr<MessageInfo> msgInfo = halServerPort.receiveMessage();
        if (uuid_compare(msgInfo->type, MSG_START_HAL_RESULT) == 0) {
            bool result;
            try {
                zbon::decode(msgInfo->data, result);
            } catch(zbon::DecoderException &e) {
                std::cout << "Received malformed MSG_START_HAL_RESULT message." << std::endl;
                continue;
            }

            if (result) {
                std::cout << "HAL startup complete. TODO: what's next?" << std::endl;
            } else {
                std::cout << "HAL startup failed." << std::endl;
            }
        } else if (uuid_compare(msgInfo->type, MSG_FOUND_CONTROLLER) == 0) {
            std::tuple<uuid_t, zbon::EncodedData> msg;
            try {
                zbon::decode(msgInfo->data, msg);
            } catch(zbon::DecoderException &e) {
                std::cout << "Received malformed MSG_FOUND_CONTROLLER message." << std::endl;
                continue;
            }

            if (uuid_compare(std::get<0>(msg), CONTROLLER_TYPE_PCI) == 0) {
                //new std::thread(ControllerServer, PCIController, std::move(std::get<1>(msg)), PCI);
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
