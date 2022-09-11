#include <map>
#include <mutex>
#include <iostream>
#include <zagtos/Interrupt.hpp>
#include <zagtos/EventQueue.hpp>
extern "C" {
    #include <acpi.h>
}

struct InstalledHandler {
    ACPI_OSD_HANDLER callback;
    void *context;
    zagtos::Interrupt interrupt;
};

static std::map<int32_t, InstalledHandler> handlers;
static std::mutex handlersLock;

void HandleInterrupt(int32_t number) {
    std::scoped_lock sl(handlersLock);

    if (!handlers.contains(number)) {
        std::cout << "OSLayer: ACPI interrupt " << number << " triggered while no handler "
                  << "registered" << std::endl;
    } else {
        InstalledHandler &handler = handlers[number];
        handler.callback(handler.context);
        handler.interrupt.processed();
    }
}

extern "C"
ACPI_STATUS AcpiOsInstallInterruptHandler(uint32_t number, ACPI_OSD_HANDLER callback, void *context) {
    std::scoped_lock sl(handlersLock);
    zagtos::Interrupt interrupt(number, zagtos::TriggerMode::RISING_EDGE);
    interrupt.subscribe(zagtos::DefaultEventQueue, number);
    handlers[number] = {callback, context, std::move(interrupt)};
    std::cout << "OSLayer: installed interrupt handler for " << number << std::endl;
    return AE_OK;
}

extern "C"
ACPI_STATUS AcpiOsRemoveInterruptHandler(uint32_t number, ACPI_OSD_HANDLER) {
    std::scoped_lock sl(handlersLock);
    handlers.erase(number);
    std::cout << "OSLayer: removed interrupt handler for " << number << std::endl;
    return AE_OK;
}

