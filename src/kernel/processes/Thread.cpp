#include <processes/Thread.hpp>
#include <processes/Process.hpp>
#include <processes/KernelThreadEntry.hpp>
#include <system/System.hpp>
#include <system/Processor.hpp>


Thread::Thread(shared_ptr<Process> process,
       UserVirtualAddress entry,
       Priority priority,
       UserVirtualAddress stackPointer,
       size_t entryArgument,
       size_t tlsPointer) :
    _ownPriority{priority},
    _currentPriority{priority},
    _state {State::Transition()},
    process{process},
    tlsPointer{tlsPointer} {

    {
        scoped_lock sl1(KernelInterruptsLock);
        scoped_lock sl(process->allThreadsLock);
        process->allThreads.append(this);
    }

    RegisterState userRegisterState(entry,
                                    stackPointer,
                                    entryArgument);

    kernelStack = make_shared<KernelStack>(userRegisterState);
    kernelEntry = UserReturnEntry;
}

Thread::Thread(const hos_v1::Thread &handOver) :
    _ownPriority{handOver.ownPriority},
    _currentPriority{handOver.currentPriority},
    _state{State::Transition()},
    tlsPointer{handOver.tlsPointer} {

    RegisterState userRegisterState(handOver.registerState);
    kernelStack = make_shared<KernelStack>(userRegisterState);
    kernelEntry = UserReturnEntry;
}

/* for kernel-only threads: */
Thread::Thread(void (*entry)(void *),
               Priority priority):
    _ownPriority{priority},
    _currentPriority{priority},
    _state {State::Transition()},
    kernelEntry{entry} {

    kernelStack = make_shared<KernelStack>(RegisterState());
}

Thread::~Thread() {
    /* Thread should either be destructed while active or directly on creation failure */
    assert(currentProcessor() == CurrentProcessor() || currentProcessor() == nullptr);
    /* don't try deleting special threads */
    assert(process);

    {
        scoped_lock sl(process->allThreadsLock);
        process->allThreads.remove(this);
    }
}

Thread::Priority Thread::ownPriority() const noexcept {
    return _ownPriority;
}

Thread::Priority Thread::currentPriority() const noexcept {

    return _currentPriority;
}

void Thread::setCurrentPriority(Thread::Priority newValue) noexcept {
    _currentPriority = newValue;
}

Thread::State Thread::state() noexcept {
    scoped_lock sl1(KernelInterruptsLock);
    scoped_lock sl(stateLock);
    return _state;
}

void Thread::setStateLocked(Thread::State newValue) noexcept {
    if (!(((newValue.kind() == Thread::TRANSITION) != (_state.kind() == Thread::TRANSITION))
           || (newValue.kind() == Thread::ACTIVE && _state.kind() == Thread::READY)
           || (newValue.kind() == Thread::READY && _state.kind() == Thread::ACTIVE))) {
        cout << "Illegal Thread state Transition from " << _state.kind()
             << " to " << newValue.kind() << endl;
        Panic();
    }
    _state = newValue;
}

void Thread::setState(Thread::State newValue) noexcept {
    scoped_lock sl1(KernelInterruptsLock);
    scoped_lock sl(stateLock);
    setStateLocked(newValue);
}

bool Thread::atomicSetState(State oldValue, State newValue) noexcept {
    scoped_lock sl1(KernelInterruptsLock);
    scoped_lock sl(stateLock);

    if (_state == oldValue) {
        setStateLocked(newValue);
        return true;
    } else {
        return false;
    }
}

uint32_t Thread::handle() const noexcept {
    assert(_handle != INVALID_HANDLE);
    return _handle;
}

void Thread::setHandle(uint32_t handle) noexcept {
    /* setHandle should only happen once, directly after thread creation */
    assert(_handle == INVALID_HANDLE);
    assert(handle != INVALID_HANDLE);

    _handle = handle;
    kernelStack->userRegisterState()->setThreadHandle(handle);
}

Processor *Thread::currentProcessor() const noexcept {
    return _currentProcessor;
}

void Thread::currentProcessor(Processor *processor) noexcept {
    _currentProcessor = processor;
}

void Thread::setKernelEntry(void (*entry)(void *), void *data) noexcept {
    kernelEntry = entry;
    kernelEntryData = data;
}

void Thread::pinToProcessor(Processor *processor) {
    assert(this == CurrentThread());

    pinnedToProcessor = processor;
    if (processor != nullptr && processor != CurrentProcessor()) {
        scoped_lock sl1(KernelInterruptsLock);
        auto &scheduler = CurrentProcessor()->scheduler;
        scoped_lock sl2(scheduler.lock);

        scheduler.removeActiveThread();
        Scheduler::schedule(this, true);

        throw DiscardStateAndSchedule(this, move(sl1), move(sl2));
    }
}

void Thread::terminate() noexcept {
    assert(KernelInterruptsLock.isLocked());

    cout << "Thread::terminate" << endl;
    while (true) {
        State localState = state();
        switch (localState.kind()) {
        case ACTIVE:
            assert(localState.currentProcessor() != CurrentProcessor());
            cout << "TODO: send IPI to terminate active thread" << endl;
            Panic();
            break;
        case READY: {
            Scheduler &scheduler = localState.currentProcessor()->scheduler;
            if (scheduler.lock.trylock()) {
                if (state() == localState) {
                    scheduler.removeOtherThread(this);
                    scheduler.lock.unlock();
                    return;
                }
            }
            break;
        }
        case TERMINATED:
            /* nothing to do here */
            cout << "Warning: one thread terminated multiple times" << endl;
            return;
        default:
            cout << "TODO\n";
            Panic();
        }
        // TODO: find good time
        CurrentSystem.time.delayMilliseconds(1);
    }
}

Thread *CurrentThread() noexcept {
    scoped_lock sl(KernelInterruptsLock);
    return CurrentProcessor()->activeThread();
}
