#include <processes/Thread.hpp>
#include <processes/Process.hpp>
#include <processes/KernelThreadEntry.hpp>
#include <system/System.hpp>
#include <system/Processor.hpp>


Thread::Thread(shared_ptr<Process> process,
       UserVirtualAddress entry,
       Priority priority,
       UserVirtualAddress stackPointer,
       UserVirtualAddress runMessageAddress,
       UserVirtualAddress tlsBase,
       UserVirtualAddress masterTLSBase,
       size_t tlsSize,
       Status &status) :
    _ownPriority{priority},
    _currentPriority{priority},
    _state {State::Transition()},
    process{process},
    tlsBase{tlsBase} {

    if (!status) {
        return;
    }

    RegisterState userRegisterState(entry,
                                    stackPointer,
                                    runMessageAddress,
                                    tlsBase,
                                    masterTLSBase,
                                    tlsSize);

    Result result = make_shared<KernelStack>(userRegisterState);
    if (!result) {
        status = result.status();
        return;
    }
    kernelStack = *result;
    kernelEntry = UserReturnEntry;
}

/* shorter constructor for non-first threads, which don't want to know info about master TLS */
Thread::Thread(shared_ptr<Process> process,
       UserVirtualAddress entry,
       Priority priority,
       UserVirtualAddress stackPointer,
       UserVirtualAddress tlsBase,
       Status &status) :
    Thread(process, entry, priority, stackPointer, stackPointer, tlsBase, 0, 0, status) {}

Thread::Thread(const hos_v1::Thread &handOver, Status &status) :
    _ownPriority{handOver.ownPriority},
    _currentPriority{handOver.currentPriority},
    _state{State::Transition()},
    tlsBase{handOver.TLSBase} {

    if (!status) {
        return;
    }

    RegisterState userRegisterState(handOver.registerState);
    Result result = make_shared<KernelStack>(userRegisterState);
    if (!result) {
        status = result.status();
        return;
    }
    kernelStack = *result;
    kernelEntry = UserReturnEntry;

    cout << "thread handover cs: " << kernelStack->userRegisterState()->cs << endl;
}

/* for kernel-only threads: */
Thread::Thread(void (*entry)(void *),
               Priority priority,
               Status &status):
    _ownPriority{priority},
    _currentPriority{priority},
    _state {State::Transition()},
    kernelEntry{entry} {

    if (!status) {
        return;
    }

    Result result = make_shared<KernelStack>(RegisterState());
    if (!result) {
        status = result.status();
        return;
    }
    kernelStack = *result;
}

Thread::~Thread() {
    /* Thread should either be destructed while active or directly on creation failure */
    assert(currentProcessor() == CurrentProcessor || currentProcessor() == nullptr);
    /* don't try deleting special threads */
    assert(process);
}

Thread::Priority Thread::ownPriority() const {
    return _ownPriority;
}

Thread::Priority Thread::currentPriority() const {

    return _currentPriority;
}

void Thread::setCurrentPriority(Thread::Priority newValue) {
    _currentPriority = newValue;
}

Thread::State Thread::state() {
    //assert(stateLock.isLocked());
    scoped_lock sl(stateLock);
    return _state;
}

void Thread::setState(Thread::State newValue) {
    scoped_lock sl(stateLock);
    assert(((newValue.kind() == Thread::TRANSITION) != (_state.kind() == Thread::TRANSITION))
           || (newValue.kind() == Thread::ACTIVE && _state.kind() == Thread::RUNNING)
           || (newValue.kind() == Thread::RUNNING && _state.kind() == Thread::ACTIVE));
    _state = newValue;
}

uint32_t Thread::handle() const {
    assert(_handle != INVALID_HANDLE);
    return _handle;
}

void Thread::setHandle(uint32_t handle) {
    /* setHandle should only happen once, directly after thread creation */
    assert(_handle == INVALID_HANDLE);
    assert(handle != INVALID_HANDLE);

    _handle = handle;
    kernelStack->userRegisterState()->setThreadHandle(handle);
}

/* The currentProcessor variable is stored on the stack for easy loding during the user-to-kernel
 * switch */
Processor *Thread::currentProcessor() const {
    return kernelStack->userRegisterState()->currentProcessor;
}

void Thread::currentProcessor(Processor *processor) {
    kernelStack->userRegisterState()->currentProcessor = processor;
}

void Thread::setKernelEntry(void (*entry)(void *), void *data) {
    kernelEntry = entry;
    kernelEntryData = data;
}

void Thread::terminate() {
    assert(CurrentProcessor->kernelInterruptsLock.isLocked());

    cout << "Thread::terminate" << endl;
    while (true) {
        State localState = state();
        switch (localState.kind()) {
        case ACTIVE:
            assert(localState.currentProcessor() != CurrentProcessor);
            cout << "TODO: send IPI to terminate active thread" << endl;
            Panic();
            break;
        case RUNNING: {
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

Thread *CurrentThread() {
    return CurrentProcessor->scheduler.activeThread();
}
