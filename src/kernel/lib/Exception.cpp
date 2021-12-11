#include <lib/Exception.hpp>
#include <processes/Process.hpp>
#include <system/Processor.hpp>

Exception::Action Exception::handleAsThreadKilled() noexcept {
    CurrentProcessor()->scheduler.lock.lock();
    CurrentProcessor()->scheduler.removeActiveThread();
    return Action::SCHEDULE;
}

Exception::Action BadUserSpace::handle() noexcept {
    try {
        process->crash("BadUserSpace Exception");
    } catch (ThreadKilled& e) {
        return handleAsThreadKilled();
    }
    return Action::RETRY;
}

const char *BadUserSpace::description() noexcept {
    return "BadUserSpace";
}

Exception::Action OutOfKernelHeap::handle() noexcept {
    cout << "TODO: handle OutOfKernelHeap" << endl;
    Panic();
}

const char *OutOfKernelHeap::description() noexcept {
    return "OutOfKernelHeap";
}

Exception::Action OutOfMemory::handle() noexcept {
    cout << "TODO: handle OutOfMemory" << endl;
    Panic();
}

const char *OutOfMemory::description() noexcept {
    return "OutOfMemory";
}

Exception::Action ThreadKilled::handle() noexcept {
    return handleAsThreadKilled();
}

const char *ThreadKilled::description() noexcept {
    return "ThreadKilled";
}

Exception::Action DiscardStateAndSchedule::handle() noexcept {
    /* The exception object is destroyed after calling this handler, which would unlock the
     * scheduler lock. Calling stick() makes it keep locked.
     * For the KernelInterruptsLock, this is not necessary, as it is a recursive lock, and
     * will be additionally locked from a different place at this point. */
    schedulerLock.stick();

    cout << "DiscardStateAndSchedule" << endl;

    /* We can't use CurrentThread() here, as users of this exception already set activeThread to
     * null. */
    thread->setKernelEntry(UserReturnEntry);
    return Action::SCHEDULE;
}

const char *DiscardStateAndSchedule::description() noexcept {
    return "DiscardStateAndSchedule";
}
