#include <iostream>
#include <tuple>
#include <zagtos/Messaging.hpp>
#include <zagtos/protocols/PS2.hpp>
#include <zagtos/Interrupt.hpp>
#include <zagtos/IOPortRange.hpp>
#include <zagtos/protocols/Controller.hpp>
#include <mutex>
#include <thread>
#include <chrono>

using namespace zagtos;

class Controller {
private:
    std::mutex lock;
    RemotePort &environmentPort;
    Interrupt firstInterrupt;
    Interrupt secondInterrupt;
    IOPortRange ports;

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

            FirstPortClock = 0x8,
            SecondPortClock = 0x10,
            Translation = 0x20,
        };
    };

    uint8_t status() {
        return ports.read(4, 1);
    }

    bool statusBit(Status bit) {
        return status() & (1u << static_cast<uint32_t>(bit));
    }

    void command(Command cmd) {
        while (statusBit(Status::InputBufferFull)) {
            /* wait */
        }
        ports.write(4, 1, static_cast<uint32_t>(cmd));
    }

    void command(Command cmd, uint8_t value) {
        while (statusBit(Status::InputBufferFull)) {
            /* wait */
        }
        ports.write(4, 1, static_cast<uint32_t>(cmd));
        while (statusBit(Status::InputBufferFull)) {
            /* wait */
        }
        ports.write(0, 1, value);
    }

    uint8_t commandWithResult(Command cmd) {
        while (statusBit(Status::InputBufferFull)) {
            /* wait */
        }
        ports.write(4, 1, static_cast<uint32_t>(cmd));
        while (!statusBit(Status::OutputBufferFull)) {
            /* wait */
        }
        return ports.read(0, 1);
    }

    bool sendToFirstDevice(uint8_t value) {
        using std::chrono::duration_cast;
        using std::chrono::system_clock;
        using std::chrono::milliseconds;

        auto startTime = system_clock::now();
        while (statusBit(Status::InputBufferFull)) {
            if (duration_cast<milliseconds>(system_clock::now() - startTime).count() > 200) {
                std::cout << "timeout" << std::endl;
                return false;
            }
        }

        ports.write(0, 1, value);
        return true;
    }

public:
    bool firstPortWorks;
    bool secondPortWorks;

    Controller(RemotePort &_environmentPort,
               Interrupt _firstInterrupt,
               Interrupt _secondInterrupt,
               IOPortRange _ports) :
        environmentPort{_environmentPort},
        firstInterrupt(std::move(_firstInterrupt)),
        secondInterrupt(std::move(_secondInterrupt)),
        ports(std::move(_ports)) {

        /* Initialization sequence from:
         * https://wiki.osdev.org/%228042%22_PS/2_Controller */

        command(Command::DisableFirstPort);
        command(Command::DisableSecondPort);

        /* read any reamaining data from output buffer */
        ports.read(0, 1);

        uint8_t config = commandWithResult(Command::ReadConifg);
        config &= ~(Config::FirstPortInterrupt|Config::SecondPortInterrupt|Config::Translation);
        command(Command::WriteConfig, config);

        if (commandWithResult(Command::SelfTest) != 0x55) {
            throw std::runtime_error("Controller did not pass self test");
        }
        /* self test may destroy config */
        command(Command::WriteConfig, config);

        bool dualChannelController = config & Config::SecondPortClock;

        firstPortWorks = commandWithResult(Command::TestFirstPort) == 0;
        if (dualChannelController) {
            secondPortWorks = commandWithResult(Command::TestFirstPort) == 0;
        } else {
            secondPortWorks = false;
        }

        if (firstPortWorks) {
            command(Command::EnableFirstPort);
            config |= Config::FirstPortInterrupt;
        }
        if (secondPortWorks) {
            command(Command::EnableSecondPort);
            config |= Config::SecondPortInterrupt;
        }
        command(Command::WriteConfig, config);
    }

    void firstPortHandler() {
        firstInterrupt.subscribe();

        std::cout << "sending reset" << std::endl;
        {
            /* reset already connected device */
            std::scoped_lock sl(lock);
            sendToFirstDevice(0xff);
        }

        bool devicePresent = false;

        while (true) {
            firstInterrupt.wait();
            std::cout << "Got interrupt!" << std::endl;

            uint8_t data = ports.read(0, 1);

            if (!devicePresent) {
                std::cout << "Got data from first device: " << data << std::endl;
            } else {
                /* TODO: send to driver */
            }
            firstInterrupt.processed();
        }
    }
};

int main() {
    using MsgType = std::tuple<RemotePort, std::tuple<Interrupt, Interrupt, IOPortRange>>;
    auto [envPort, tuple] = decodeRunMessage<MsgType>(controller::MSG_START);

    std::cout << "Hello" << std::endl;

    Controller controller(envPort,
                          std::move(std::get<0>(tuple)),
                          std::move(std::get<1>(tuple)),
                          std::move(std::get<2>(tuple)));

    if (!controller.firstPortWorks && !controller.secondPortWorks) {
        throw std::runtime_error("No working ports!");
    }

    std::thread thread1;

    if (controller.firstPortWorks) {
        std::cout << "Port 1 works" << std::endl;
        thread1 = std::thread(&Controller::firstPortHandler, &controller);
        thread1.join();
    }

}
