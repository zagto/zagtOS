#ifndef THREAD_HPP
#define THREAD_HPP

#include <interrupts/RegisterState.hpp>
#include <memory>
#include <vector>

class Processor;
class Scheduler;
class Process;
class Port;

static const size_t THREAD_STRUCT_AREA_SIZE = 0x400;


class Thread {
protected:
    friend class Scheduler;
    Processor *currentProcessor{nullptr};
    bool usesFloatingPoint{false};

public:
    enum Priority {
        IDLE, BACKGROUND, FOREGROUND, INTERACTIVE_FOREGROUND,
    };
    static const size_t NUM_PRIORITIES = 4;
    static const size_t KEEP_PRIORITY = 0xffff'ffff;


public:
    RegisterState registerState;
    shared_ptr<Process> process;
    UserVirtualAddress tlsBase;
    Priority ownPriority;
    Priority currentPriority;

    Thread(shared_ptr<Process> process,
           VirtualAddress entry,
           Priority priority,
           UserVirtualAddress stackPointer,
           UserVirtualAddress tlsBase,
           UserVirtualAddress masterTLSBase,
           size_t tlsSize) :
        registerState(entry, stackPointer, tlsBase, masterTLSBase, tlsSize),
        process{process},
        tlsBase{tlsBase},
        ownPriority{priority},
        currentPriority{priority} {}
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
    bool handleSyscall(shared_ptr<Thread> self);
};

#endif
