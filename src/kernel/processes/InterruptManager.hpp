#pragma once

#include <interrupts/Interrupts.hpp>
#include <processes/ThreadList.hpp>
#include <processes/Thread.hpp>
#include <vector>

class ProcessInterrupt;

class PlatformInterrupt {
private:
    size_t processorID{-1ul};
    size_t vectorNumber{-1ul};

    uint64_t occurence;
    threadList::List<&Thread::interruptReceptor> waitingThreads;
    size_t totalSubscribers{0};
    size_t processingSubscribers{0};

public:
    SpinLock lock;
    void initialize(size_t processorID, size_t vectorNumber) noexcept;
    void subscribe(ProcessInterrupt &interrupt);
    void unsubscribe(ProcessInterrupt &interrupt);
    void removeWaiting(ProcessInterrupt &interrupt) noexcept;
    void processed(ProcessInterrupt &interrupt);
    void wait(ProcessInterrupt &interrupt);
    void occur() noexcept;
};

class ProcessInterrupt {
private:
    friend class PlatformInterrupt;
    Thread *subscribedThread{nullptr};
    //bool waiting{false};
    uint64_t processedOccurence{0};

public:
    PlatformInterrupt &platformInterrupt;

    ProcessInterrupt() noexcept;
    ~ProcessInterrupt();
    ProcessInterrupt(ProcessInterrupt &) = delete;
    ProcessInterrupt operator=(ProcessInterrupt &) = delete;
};

namespace interruptManager {
    class InterruptManagerClass {
    private:
        /* uppercase Vector - interrupt vector
         * lowercase vector - dynamic array data structure */
        vector<vector<PlatformInterrupt>> platformInterrupts;
        static constexpr size_t vectorOffset = DynamicInterruptRegion.start;

    public:
        InterruptManagerClass();

        PlatformInterrupt &getAny() noexcept;
        void occur(size_t processorID, size_t vectorNumber) noexcept;
    };

    extern InterruptManagerClass InterruptManager;
}

using interruptManager::InterruptManager;
