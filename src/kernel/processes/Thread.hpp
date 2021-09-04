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
        static State Futex(FutexManager *manager, uint64_t futexID) {
            return State(Thread::FUTEX, reinterpret_cast<size_t>(manager), futexID);
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

protected:
    /* State */
    friend class Scheduler;
    friend class Process;
    template <threadList::Receptor Thread::*>
    friend class threadList::List;

    threadList::Receptor ownerReceptor;
    threadList::Receptor processReceptor;

    void (*kernelEntry)(void *){nullptr};
    void *kernelEntryData;
public:
    //RegisterState *registerState{nullptr};
    //VectorRegisterState vectorState;
    shared_ptr<KernelStack> kernelStack;
    shared_ptr<Process> process;
    UserVirtualAddress tlsBase;

    Thread(shared_ptr<Process> process,
           UserVirtualAddress entry,
           Priority priority,
           UserVirtualAddress stackPointer,
           UserVirtualAddress runMessageAddress,
           UserVirtualAddress tlsBase,
           UserVirtualAddress masterTLSBase,
           size_t tlsSize,
           Status &);
    /* shorter constructor for non-first threads, which don't want to know info about master TLS */
    Thread(shared_ptr<Process> process,
           UserVirtualAddress entry,
           Priority priority,
           UserVirtualAddress stackPointer,
           UserVirtualAddress tlsBase,
           Status &status);
    /* for kernel-only threads: */
    Thread(void (*entry)(void *),
           Priority priority,
           Status &status);
    /* constructor used during kernel handover. process and handle fields need to be inserted
     * later. */
    Thread(const hos_v1::Thread &handOver, Status &);

    ~Thread();

    UserVirtualAddress threadLocalStorage() {
        // having a TLS is optional, otherwise tlsBase is null
        if (tlsBase.value()) {
            return UserVirtualAddress(tlsBase.value() - hos_v1::THREAD_STRUCT_AREA_SIZE);
        } else {
            return {};
        }
    }

    Priority ownPriority() const;
    Priority currentPriority() const;
    void setCurrentPriority(Priority newValue);
    State state();
    void setState(State newValue);
    Processor *currentProcessor() const;
    void currentProcessor(Processor *processor);
    void setHandle(uint32_t handle);
    uint32_t handle() const;
    void setKernelEntry(void (*entry)(void *), void *data = nullptr);

    /* danger zone - only call this while holding no locks on potential owners. This is for
     * scenarios, like exit, kill ... and puts the thread in EXIT state. */
    void terminate();
};

Thread *CurrentThread();
