#pragma once

#include <mutex>
#include <zagtos/Messaging.hpp>
#include <zagtos/IOPortRange.hpp>
#include "Port.hpp"

namespace ps2controller {

class Controller {
private:
    static constexpr uint16_t IO_PORT_DATA = 0;
    static constexpr uint16_t IO_PORT_COMMAND = 4;
    static constexpr uint16_t IO_PORT_STATUS = 4;

    static constexpr uint32_t DATA_WRITE_TIMEOUT_MS = 200;

    zagtos::RemotePort &environmentPort;
    zagtos::IOPortRange ioPorts;

public:
    enum class Command {
        ReadConifg = 0x20,
        WriteConfig = 0x60,

        DisableSecondPort = 0xa7,
        EnableSecondPort = 0xa8,

        TestSecondPort = 0xa9,
        SelfTest = 0xaa,
        TestFirstPort = 0xab,

        DisableFirstPort = 0xad,
        EnableFirstPort = 0xae,

        DataToSecondPort = 0xd4,
    };

    enum class Status {
        OutputBufferFull,
        InputBufferFull,
        SystemFlag,
        InputIsForController,
        Reserved1,
        Reserved2,
        TimeoutError,
        ParityError,
    };

    struct Config {
        enum : uint8_t {
            FirstPortInterrupt = 0x0,
            SecondPortInterrupt = 0x1,
            SystemFlag = 0x2,

            FirstPortClock = 0x10,
            SecondPortClock = 0x20,
            Translation = 0x40,
        };
    };

    std::mutex lock;
    std::array<Port, 2> ports;

    Controller(zagtos::RemotePort &_environmentPort,
               zagtos::Interrupt firstInterrupt,
               zagtos::Interrupt secondInterrupt,
               zagtos::IOPortRange ioPorts);
    Controller(Controller &) = delete;

    uint8_t status();
    bool statusBit(Status bit);
    void command(Command cmd);
    void data(uint8_t value);
    uint8_t data();
    uint8_t commandWithResult(Command cmd);
};

}
