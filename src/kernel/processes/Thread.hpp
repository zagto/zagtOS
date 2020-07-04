#ifndef THREAD_HPP
#define THREAD_HPP

#include <interrupts/RegisterState.hpp>
#include <memory>
#include <vector>
#include <utility>

class Processor;
class Scheduler;
class Process;
class Port;
class FutexManager;

static const size_t THREAD_STRUCT_AREA_SIZE = 0x400;


class Thread {
public:
    enum Priority {
        IDLE, BACKGROUND, FOREGROUND, INTERACTIVE_FOREGROUND,
    };
    enum {
        ACTIVE, RUNNING, MESSAGE, FUTEX, TRANSITION, TERMINATED
    };
    class State {
    private:
        uint32_t _kind;
        size_t relatedObject;
        size_t relatedObject2;

        State(uint32_t kind, size_t _relatedObject = 0, size_t _relatedObject2 = 0):
            _kind{kind},
            relatedObject{_relatedObject},
            relatedObject2{_relatedObject2} {}

    public:
        static State Active(Processor *processor) {
            return State(Thread::ACTIVE, reinterpret_cast<size_t>(processor));
        }
        static State Running(Processor *processor) {
            return State(Thread::RUNNING, reinterpret_cast<size_t>(processor));
        }
        static State WaitMessage(Port *port) {
            return State(Thread::MESSAGE, reinterpret_cast<size_t>(port));
        }
        static State Futex(FutexManager *manager, PhysicalAddress address) {
            return State(Thread::FUTEX, reinterpret_cast<size_t>(manager), address.value());
        }
        static State Transition() {
            return State(Thread::TRANSITION);
        }
        static State Terminated() {
            return State(Thread::TERMINATED);
        }

        uint32_t kind() const {
            return _kind;
        }
        Processor *currentProcessor() {
            assert(_kind == ACTIVE || _kind == RUNNING);
            return reinterpret_cast<Processor *>(relatedObject);
        }
        pair<FutexManager &, PhysicalAddress> currentFutex();
        Port &currentPort();
        bool operator==(const State &other) const {
            return _kind == other._kind && relatedObject == other.relatedObject && relatedObject2 == other.relatedObject2;
        }
    };

    /* Owners:
     * - RUNNING, WAITING: Scheduler->lock
     * - FUTEX*: FutexManager->lock
     * - MESSAGE: Port*/

    static const size_t NUM_PRIORITIES = 4;
    static const size_t KEEP_PRIORITY = 0xffff'ffff;

protected:
    /* State */
    friend class Scheduler;
    Processor *_currentProcessor{nullptr};

    /* stuff only to be changed with stateLock aquired */
    Thread *previous;
    Thread *next;
private:
    Priority _ownPriority;
    Priority _currentPriority;
    State _state;

    mutex stateLock;

public:
    RegisterState registerState;
    shared_ptr<Process> process;
    UserVirtualAddress tlsBase;

    Thread(shared_ptr<Process> process,
           VirtualAddress entry,
           Priority priority,
           UserVirtualAddress stackPointer,
           UserVirtualAddress tlsBase,
           UserVirtualAddress masterTLSBase,
           size_t tlsSize) :
        previous{nullptr},
        next{nullptr},
        _ownPriority{priority},
        _currentPriority{priority},
        _state {State::Transition()},
        registerState(entry, stackPointer, tlsBase, masterTLSBase, tlsSize),
        process{process},
        tlsBase{tlsBase} {}
    /* shorter constructor for non-first threads, which don't want to know info about master TLS */
    Thread(shared_ptr<Process> process,
           VirtualAddress entry,
           Priority priority,
           UserVirtualAddress stackPointer,
           UserVirtualAddress tlsBase) :
        Thread(process, entry, priority, stackPointer, tlsBase, 0, 0) {}

    ~Thread();

    UserVirtualAddress threadLocalStorage() {
        // having a TLS is optional, otherwise tlsBase is null
        if (tlsBase.value()) {
            return UserVirtualAddress(tlsBase.value() - THREAD_STRUCT_AREA_SIZE);
        } else {
            return {};
        }
    }
    bool handleSyscall();

    Priority ownPriority() const;
    Priority currentPriority() const;
    void setCurrentPriority(Priority newValue);
    State state();
    void setState(State newValue);
    Processor *currentProcessor() const;

    /* danger zone - only call this while holding no locks on potential owners. This is for
     * scenarios, like exit, kill ... and puts the thread in EXIT state. */
    void terminate() noexcept;
};

#endif
