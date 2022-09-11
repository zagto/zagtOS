#include <chrono>
#include <optional>
#include "Port.hpp"
#include "Controller.hpp"

namespace ps2controller {

Port::Port(Controller &controller, zagtos::Interrupt interrupt, size_t portIndex) :
    controller{controller},
    portIndex{portIndex},
    interrupt{std::move(interrupt)} {}

void Port::sendToDevice(uint8_t value) {
    if (portIndex == 1) {
        controller.command(Controller::Command::DataToSecondPort);
    }
    controller.data(value);
}

void Port::handler() {
    /* reset already connected device */
    sendToDevice(0xff);

    bool devicePresent = false;
    std::vector<uint8_t> data;

    while (true) {
        //interrupt.wait();
        std::cout << "Got interrupt " << portIndex << std::endl;

        std::optional<uint8_t> byte;

        {
            if (controller.statusBit(Controller::Status::OutputBufferFull)) {
                std::cout << "got byte" << std::endl;
                byte = controller.data();
            } else {
                std::cout << "interrupt " << portIndex << " but no new data" << std::endl;
            }
        }

        if (!devicePresent) {
            std::cout << "Got data from first device " << portIndex << ": " << static_cast<uint64_t>(*byte) << std::endl;
        } else {
            /* TODO: send to driver */
        }
        interrupt.processed();
    }
}

}
