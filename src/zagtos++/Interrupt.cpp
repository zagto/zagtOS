#include <zagtos/Interrupt.hpp>
#include <zagtos/syscall.h>
#include <zagtos/EventQueue.hpp>
#include <memory>

namespace zagtos {

Interrupt::Interrupt(uint32_t fixedNumber, TriggerMode triggerMode) {
    _handle = zagtos_syscall3(SYS_CREATE_INTERRUPT,
                              CREATE_FIXED,
                              fixedNumber,
                              static_cast<size_t>(triggerMode));
}

Interrupt::Interrupt(TriggerMode triggerMode, ProcessorInterruptInfo &processorInterruptInfo) {
    _handle = zagtos_syscall3(SYS_CREATE_INTERRUPT,
                              CREATE_PROCESSOR_DIRECT,
                              reinterpret_cast<size_t>(&processorInterruptInfo),
                              static_cast<size_t>(triggerMode));
}

Interrupt &Interrupt::operator=(Interrupt &&other) {
    HandleObject::operator=(std::move(other));
    return *this;
}

void Interrupt::subscribe(EventQueue &eventQueue, size_t eventTag) {
    zagtos_syscall3(SYS_SUBSCRIBE_INTERRUPT, _handle, eventQueue._handle, eventTag);
}

void Interrupt::unsubscribe() {
    zagtos_syscall1(SYS_UNSUBSCRIBE_INTERRUPT, _handle);
}

void Interrupt::processed() {
    zagtos_syscall1(SYS_PROCESSED_INTERRUPT, _handle);
}

}
