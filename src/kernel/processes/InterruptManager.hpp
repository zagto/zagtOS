#pragma once

#include <interrupts/Interrupts.hpp>
#include <processes/ThreadList.hpp>
#include <processes/Thread.hpp>
#include <vector>


struct ProcessorInterrupt {
    size_t processorID{-1ul};
    size_t vectorNumber{-1ul};
};

namespace interruptManager {
class Manager;
}

class BoundInterrupt {
private:
    friend class System;
    friend class apic::IOAPIC;
    friend class interruptManager::Manager;

    struct Subscription {
        shared_ptr<Process> process;
        uint64_t processedOccurence{0};
    };

    SpinLock lock;
    ProcessorInterrupt processorInterrupt;

    InterruptType type;
    size_t typeData;
    TriggerMode triggerMode;

    uint64_t occurence{0};
    size_t processingSubscribers{0};
    threadList::List<&Thread::interruptReceptor> waitingThreads;
    vector<Subscription> subscriptions;

    vector<Subscription>::Iterator findSubscription(shared_ptr<Process> process);
    void unsubscribe(vector<Subscription>::Iterator subscription) noexcept;

public:
    BoundInterrupt(InterruptType type, size_t typeData, TriggerMode triggerMode);
    ~BoundInterrupt() noexcept;
    BoundInterrupt(BoundInterrupt &) = delete;

    void subscribe();
    void unsubscribe();
    void removeWaiting() noexcept;
    void processed();
    void wait();
    void occur() noexcept;
    void checkFullyProcessed() noexcept;
};

namespace interruptManager {

class Manager {
    private:
        static constexpr size_t vectorOffset = DynamicInterruptRegion.start;        

        SpinLock lock;
        vector<vector<BoundInterrupt *>> allInterrupts;

        PlatformInterrupt findFree() noexcept;

    public:
        Manager();

        void bind(BoundInterrupt *binding);
        void unbind(BoundInterrupt *binding) noexcept;
        void occur(ProcessorInterrupt processorInterrupt) noexcept;
    };

    extern Manager InterruptManager;
}

using interruptManager::InterruptManager;
