#include <syscalls/WaitForEvent.hpp>
#include <system/Processor.hpp>
#include <processes/EventQueue.hpp>


size_t WaitForEvent(const shared_ptr<Process> &process,
                   size_t queueHandle,
                   size_t eventInfoAddress,
                   size_t,
                   size_t,
                   size_t) {
    auto eventQueue = process->handleManager.lookup<shared_ptr<EventQueue>>(queueHandle);

    scoped_lock sl2(eventQueue->lock);

    unique_ptr<Event> event = eventQueue->getEvent();
    if (!event) {
        /* no pending events, Thread has to wait */
        Processor *processor = CurrentProcessor();
        Thread *thread = CurrentThread();
        scoped_lock sl(KernelInterruptsLock);
        scoped_lock sl3(processor->scheduler.lock);
        processor->scheduler.removeActiveThread();
        thread->setState(Thread::State::Event(eventQueue.get(), eventInfoAddress));
        thread->kernelStack->userRegisterState()->setSyscallResult(0);
        eventQueue->wait(thread);
        throw DiscardStateAndSchedule(thread, move(sl), move(sl3));
    }

    event->writeEventInfo(process, eventInfoAddress);
    return 0;
}
