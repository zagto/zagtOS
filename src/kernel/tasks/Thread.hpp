#ifndef THREAD_HPP
#define THREAD_HPP

#include <interrupts/RegisterState.hpp>

class Processor;
class Scheduler;
class Task;

static const size_t THREAD_STRUCT_AREA_SIZE = 0x400;


class Thread {
protected:
    friend class Scheduler;
    Processor *currentProcessor{nullptr};
    bool usesFloatingPoint{false};

public:
    enum Priority {
        IDLE, BACKGROUND, FOREGROUND, INTERACTIVE_BACKGROUND, INTERACTIVE_FOREGROUND
    };
    static const size_t NUM_PRIORITIES = 5;

public:
    RegisterState registerState;
    Task *task;
    UserVirtualAddress tlsBase;
    Priority ownPriority;
    Priority currentPriority;

    Thread(Task *task,
           VirtualAddress entry,
           Priority priority,
           UserVirtualAddress stackPointer,
           UserVirtualAddress tlsBase,
           UserVirtualAddress masterTLSBase,
           size_t tlsSize) :
        registerState(entry, stackPointer, tlsBase, masterTLSBase, tlsSize),
        task{task},
        tlsBase{tlsBase},
        ownPriority{priority},
        currentPriority{priority} {}
    ~Thread();

    UserVirtualAddress userThreadStruct() {
        return UserVirtualAddress(tlsBase.value() - THREAD_STRUCT_AREA_SIZE);
    }
    bool handleSyscall();
};

#endif
