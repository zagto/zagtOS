#include <chrono>
#include <unistd.h>
#include "Controller.hpp"

namespace ps2controller {

Controller::Controller(zagtos::RemotePort &_environmentPort,
                       zagtos::Interrupt firstInterrupt,
                       zagtos::Interrupt secondInterrupt,
                       zagtos::IOPortRange _ioPorts) :
    environmentPort{_environmentPort},
    ioPorts(std::move(_ioPorts)),
    ports{Port(*this, std::move(firstInterrupt), 0), Port(*this, std::move(secondInterrupt), 1)} {

    /* Initialization sequence from:
     * https://wiki.osdev.org/%228042%22_PS/2_Controller */

    command(Command::DisableFirstPort);
    command(Command::DisableSecondPort);
    std::cout << "read " << std::endl;

    /* read any reamaining data from output buffer */
    while (statusBit(Status::OutputBufferFull)) {
        ioPorts.read(IO_PORT_DATA, 1);
    }
    std::cout << "config " << std::endl;

    uint8_t config = commandWithResult(Command::ReadConifg);
    config &= ~(Config::FirstPortInterrupt|Config::SecondPortInterrupt|Config::Translation);
    command(Command::WriteConfig);
    data(config);

    std::cout << "self test " << std::endl;

    if (commandWithResult(Command::SelfTest) != 0x55) {
        throw std::runtime_error("Controller did not pass self test");
    }
    std::cout << "self test end" << std::endl;
    /* self test may destroy config */
    command(Command::WriteConfig);
    data(config);

    bool dualChannelController = config & Config::SecondPortClock;

    ports[0].works = commandWithResult(Command::TestFirstPort) == 0;
    if (dualChannelController) {
        ports[1].works = commandWithResult(Command::TestFirstPort) == 0;
    }

    ports[0].interrupt.subscribe();
    ports[1].interrupt.subscribe();

    if (ports[0].works) {
        command(Command::EnableFirstPort);
        config |= Config::FirstPortInterrupt;
    }
    if (ports[1].works) {
        command(Command::EnableSecondPort);
        config |= Config::SecondPortInterrupt;
    }
    command(Command::WriteConfig);
    data(config);
}

uint8_t Controller::status() {
    return ioPorts.read(IO_PORT_STATUS, 1);
}

bool Controller::statusBit(Status bit) {
    return status() & (1u << static_cast<uint32_t>(bit));
}

void Controller::command(Command cmd) {
    while (statusBit(Status::InputBufferFull)) {
        /* wait */
    }
    ioPorts.write(IO_PORT_COMMAND, 1, static_cast<uint32_t>(cmd));
}

void Controller::data(uint8_t value) {
    using std::chrono::duration_cast;
    using std::chrono::system_clock;
    using std::chrono::milliseconds;


    auto startTime = system_clock::now();
    while (statusBit(Status::InputBufferFull)) {
        auto duration = duration_cast<milliseconds>(system_clock::now() - startTime).count();
        if (duration > DATA_WRITE_TIMEOUT_MS) {
            throw std::logic_error("data write timeout");
        }
    }
    ioPorts.write(IO_PORT_DATA, 1, value);
}

uint8_t Controller::data() {
    return ioPorts.read(IO_PORT_DATA, 1);
}

uint8_t Controller::commandWithResult(Command cmd) {
    while (statusBit(Status::InputBufferFull)) {
        /* wait */
    }
    ioPorts.write(IO_PORT_COMMAND, 1, static_cast<uint32_t>(cmd));
    while (!statusBit(Status::OutputBufferFull)) {
        /* wait */
    }
    return ioPorts.read(IO_PORT_DATA, 1);
}

}
