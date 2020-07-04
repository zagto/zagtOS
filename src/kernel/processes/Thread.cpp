#include <processes/Thread.hpp>
#include <processes/Process.hpp>
#include <system/System.hpp>


Thread::~Thread() {
    assert(_currentProcessor == CurrentProcessor);
    /* don't try deleting special threads */
    assert(process);
}

Thread::Priority Thread::ownPriority() const {
    assert(stateLock.isLocked());
    return _ownPriority;
}

Thread::Priority Thread::currentPriority() const {
    assert(stateLock.isLocked());
    return _currentPriority;
}

void Thread::setCurrentPriority(Thread::Priority newValue) {
    assert(stateLock.isLocked());
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

Processor *Thread::currentProcessor() const {
    assert(stateLock.isLocked());
    return _currentProcessor;
}

bool Thread::removeFromOwner() noexcept {
    while (true) {
        State localState = state();
        switch (localState.kind()) {
        case ACTIVE:
            cout << "TODO: send IPI to terminate active thread" << endl;
            break;
        case RUNNING:
            Scheduler &scheduler = localState.currentProcessor()->scheduler;
            if (scheduler.lock.trylock()) {
                if (state() == localState) {
                    scheduler.remove(this);
                    scheduler.lock.unlock();
                    return true;
                }
            }
        }
    }
}
