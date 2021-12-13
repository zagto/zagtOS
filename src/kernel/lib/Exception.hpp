#pragma once

#include <memory>
#include <mutex>

class Process;
class Thread;

class Exception {
public:
    /* also defined in ContextSwitch.asm */
    enum class Action : size_t {
        RETRY=1, CONTINUE=2, SCHEDULE=3
    };

    virtual Action handle() noexcept = 0;
    virtual const char *description() noexcept = 0;
    Action handleAsThreadKilled() noexcept;
};

class BadUserSpace : public Exception {
public:
    shared_ptr<Process> process;

    BadUserSpace(shared_ptr<Process> process) :
        process{process} {}

    Action handle() noexcept override;
    virtual const char *description() noexcept override;
};

class OutOfKernelHeap : public Exception {
public:
    OutOfKernelHeap() {}

    Action handle() noexcept override;
    virtual const char *description() noexcept override;
};

class OutOfMemory : public Exception {
public:
    OutOfMemory() {}

    Action handle() noexcept override;
    virtual const char *description() noexcept override;
};

class ThreadKilled : public Exception {
public:
    ThreadKilled() {}

    Action handle() noexcept override;
    virtual const char *description() noexcept override;
};

class DiscardStateAndSchedule : public Exception {
public:
    Thread *thread;
    scoped_lock<KernelInterruptsLockClass> kernelInterruptsLock;
    scoped_lock<SpinLock> schedulerLock;
    DiscardStateAndSchedule(Thread *thread,
                            scoped_lock<KernelInterruptsLockClass> kernelInterruptsLock,
                            scoped_lock<SpinLock> schedulerLock) noexcept :
        thread{thread},
        kernelInterruptsLock(move(kernelInterruptsLock)),
        schedulerLock(move(schedulerLock)) {
    }

    Action handle() noexcept override;
    virtual const char *description() noexcept override;
};
