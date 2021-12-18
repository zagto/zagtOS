#include <vector>
#include <mutex>
#include <memory>
#include <thread>
#include <iostream>
#include <algorithm>
#include <zagtos/Interrupt.hpp>
extern "C" {
    #include <acpi.h>
}

struct Handler {
    uint32_t number;
    ACPI_OSD_HANDLER callback;
    void *context;
    std::unique_ptr<std::thread> thread;
    zagtos::Interrupt interrupt;
};

std::vector<std::unique_ptr<Handler>> handlers;
std::mutex handlersLock;

static void handlerThread(Handler &info) {
    std::cout << "start handler Thread for interrupt " << info.number << std::endl;

    while (true) {
        if (info.interrupt.wait()) {
            info.callback(info.context);
            info.interrupt.processed();
        } else {
            /* unsubscribed */
            return;
        }
    }
}

extern "C"
ACPI_STATUS AcpiOsInstallInterruptHandler(uint32_t number, ACPI_OSD_HANDLER callback, void *context) {
    std::scoped_lock sl(handlersLock);

    auto handler = std::make_unique<Handler>();
    handler->number = number;
    handler->callback = callback;
    handler->context = context;

    handler->interrupt = zagtos::Interrupt(number, zagtos::TriggerMode::Edge, zagtos::Polarity::ActiveHigh);
    handler->interrupt.subscribe();

    handler->thread = std::make_unique<std::thread>(handlerThread, std::ref(*handler));
    handlers.push_back(std::move(handler));

    return AE_OK;
}

extern "C"
ACPI_STATUS AcpiOsRemoveInterruptHandler(uint32_t number, ACPI_OSD_HANDLER callback) {
    std::scoped_lock sl(handlersLock);

    auto iterator = std::find_if(handlers.begin(),
                                 handlers.end(),
                                 [number, callback](auto &handler) {
        return handler->number == number && handler->callback == callback;
    });
    assert(iterator != handlers.end());

    (*iterator)->interrupt.unsubscribe();
    handlers.erase(iterator);

    std::cout << "removed interrupt handler for " << number << std::endl;
    return AE_OK;
}

