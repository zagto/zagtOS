#include <zagtos/Interrupt.hpp>
#include <zagtos/syscall.h>
#include <memory>

namespace zagtos {

Interrupt::Interrupt(uint32_t fixedNumber, TriggerMode triggerMode) {
    _handle = zagtos_syscall3(SYS_CREATE_INTERRUPT,
                              CREATE_FIXED,
                              fixedNumber,
                              static_cast<size_t>(triggerMode));
}

Interrupt &Interrupt::operator=(Interrupt &&other) {
    HandleObject::operator=(std::move(other));
    return *this;
}

void Interrupt::subscribe() {
    zagtos_syscall1(SYS_SUBSCRIBE_INTERRUPT, _handle);
}

void Interrupt::unsubscribe() {
    zagtos_syscall1(SYS_UNSUBSCRIBE_INTERRUPT, _handle);
}

bool Interrupt::wait() {
    return zagtos_syscall1(SYS_WAIT_INTERRUPT, _handle);
}

void Interrupt::processed() {
    zagtos_syscall1(SYS_PROCESSED_INTERRUPT, _handle);
}

}
