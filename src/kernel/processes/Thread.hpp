#pragma once

#include <processes/KernelStack.hpp>
#include <processes/ThreadList.hpp>
#include <memory>
#include <vector>
#include <utility>

class Processor;
class Scheduler;
class Process;
class Port;
class FutexManager;


class Thread {
public:
    using Priority = hos_v1::ThreadPriority;
    enum {
        ACTIVE, RUNNING, MESSAGE, FUTEX, INTERRUPT, TRANSITION, TERMINATED
    };
    class State {
    private:
        uint32_t _kind;
        size_t relatedObject;
        size_t relatedObject2;

        State(uint32_t kind, size_t _relatedObject = 0, size_t _relatedObject2 = 0) noexcept :
            _kind{kind},
            relatedObject{_relatedObject},
            relatedObject2{_relatedObject2} {}

    public:
        static State Active(Processor *processor) noexcept {
            return State(Thread::ACTIVE, reinterpret_cast<size_t>(processor));
        }
        static State Running(Processor *processor) noexcept {
            return State(Thread::RUNNING, reinterpret_cast<size_t>(processor));
        }
        static State WaitMessage() noexcept {
            return State(Thread::MESSAGE);
        }
        static State Futex(FutexManager *manager, uint64_t futexID) noexcept {
            return State(Thread::FUTEX, reinterpret_cast<size_t>(manager), futexID);
        }
        static State Interrupt(BoundInterrupt *interrupt) noexcept {
            return State(Thread::INTERRUPT, reinterpret_cast<size_t>(interrupt));
        }
        static State Transition() noexcept {
            return State(Thread::TRANSITION);
        }
        static State Terminated() noexcept {
            return State(Thread::TERMINATED);
        }

        uint32_t kind() const noexcept {
            return _kind;
        }
        Processor *currentProcessor() noexcept {
            assert(_kind == ACTIVE || _kind == RUNNING);
            return reinterpret_cast<Processor *>(relatedObject);
        }
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
    static const uint32_t INVALID_HANDLE = -1;

private:
    const Priority _ownPriority;
    Priority _currentPriority;
    State _state;
    SpinLock stateLock;
    uint32_t _handle{INVALID_HANDLE};
    Processor *_currentProcessor = nullptr;
    Processor *pinnedToProcessor = nullptr;

protected:
    /* State */
    friend class Scheduler;
    friend class Process;
    friend class BoundInterrupt;
    template <threadList::Receptor Thread::*>
    friend class threadList::List;

    threadList::Receptor ownerReceptor;
    threadList::Receptor processReceptor;
    threadList::Receptor interruptReceptor;

    void (*kernelEntry)(void *){nullptr};
    void *kernelEntryData;
public:
    //RegisterState *registerState{nullptr};
    //VectorRegisterState vectorState;
    shared_ptr<KernelStack> kernelStack;
    shared_ptr<Process> process;
    size_t tlsPointer;

    Thread(shared_ptr<Process> process,
           UserVirtualAddress entry,
           Priority priority,
           UserVirtualAddress stackPointer,
           size_t entryArgument,
           size_t tlsPointer);
    /* shorter constructor for non-first threads, which don't want to know info about master TLS */
    Thread(shared_ptr<Process> process,
           UserVirtualAddress entry,
           Priority priority,
           UserVirtualAddress stackPointer,
           UserVirtualAddress tlsBase);
    /* for kernel-only threads: */
    Thread(void (*entry)(void *),
           Priority priority);
    /* constructor used during kernel handover. process and handle fields need to be inserted
     * later. */
    Thread(const hos_v1::Thread &handOver);

    ~Thread();

    Priority ownPriority() const noexcept;
    Priority currentPriority() const noexcept;
    void setCurrentPriority(Priority newValue) noexcept;
    State state() noexcept;
    void setState(State newValue) noexcept;
    Processor *currentProcessor() const noexcept;
    void currentProcessor(Processor *processor) noexcept;
    void setHandle(uint32_t handle) noexcept;
    uint32_t handle() const noexcept;
    void setKernelEntry(void (*entry)(void *), void *data = nullptr) noexcept;
    void pinToProcessor(Processor *processor);

    /* danger zone - only call this while holding no locks on potential owners. This is for
     * scenarios, like exit, kill ... and puts the thread in EXIT state. */
    void terminate() noexcept;
};

Thread *CurrentThread() noexcept;
