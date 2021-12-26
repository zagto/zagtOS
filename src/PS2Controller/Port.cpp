#include "Port.hpp"
#include "Controller.hpp"
#include <mutex>

namespace ps2controller {

Port::Port(Controller &controller, zagtos::Interrupt interrupt, size_t portIndex) :
    controller{controller},
    portIndex{portIndex},
    interrupt{std::move(interrupt)} {}

void Port::sendToDevice(uint8_t value) {
    std::scoped_lock sl(controller.lock);
    if (portIndex == 1) {
        controller.command(Controller::Command::DataToSecondPort);
    }
    controller.data(value);
}

void Port::handler() {
    /* reset already connected device */
    sendToDevice(0xff);

    bool devicePresent = false;

    while (true) {
        interrupt.wait();
        std::cout << "Got interrupt " << portIndex << std::endl;

        uint8_t data;
        {
            std::scoped_lock sl(controller.lock);
            data = controller.data();
        }

        if (!devicePresent) {
            std::cout << "Got data from first device " << portIndex << ": " << static_cast<uint64_t>(data) << std::endl;
        } else {
            /* TODO: send to driver */
        }
        interrupt.processed();
    }
}

}
