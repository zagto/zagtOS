#include <processes/Thread.hpp>
#include <processes/Process.hpp>
#include <system/System.hpp>


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
    registerState.setThreadHandle(handle);
}

Processor *Thread::currentProcessor() const {
    return registerState.currentProcessor;
}

void Thread::currentProcessor(Processor *processor) {
    registerState.currentProcessor = processor;
}

void Thread::terminate() noexcept {
    cout << "Thread::terminate" << endl;
    while (true) {
        State localState = state();
        switch (localState.kind()) {
        case ACTIVE:
            if (localState.currentProcessor() == CurrentProcessor) {
                Scheduler &scheduler = localState.currentProcessor()->scheduler;
                if (scheduler.lock.trylock()) {
                    if (state() == localState) {
                        scheduler.removeLocked(this);
                        scheduler.lock.unlock();
                        return;
                    }
                }
            } else {
                cout << "TODO: send IPI to terminate active thread" << endl;
                Panic();
            }
            break;
        case RUNNING: {
            Scheduler &scheduler = localState.currentProcessor()->scheduler;
            if (scheduler.lock.trylock()) {
                if (state() == localState) {
                    scheduler.removeLocked(this);
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
