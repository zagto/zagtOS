#include <common/common.hpp>
#include <system/System.hpp>
#include <processes/Port.hpp>
#include <processes/Thread.hpp>
#include <processes/Scheduler.hpp>
#include <processes/Process.hpp>

Port::Port(const shared_ptr<Process> process) noexcept :
    process{process} {}

Port::Port(const hos_v1::Port &handOver,
           const vector<shared_ptr<Thread>> &allThreads) {
    if (handOver.threadWaits) {
        waitingThread = allThreads[handOver.waitingThreadID].get();
    }
    for (size_t index = 0; index < handOver.numMessages; index++) {
        auto msg = make_unique<Message>(handOver.messages[index]);
        messages.push_back(move(msg));
    }
}

Port::~Port() {
    assert(waitingThread == nullptr);
    if (!messages.empty()) {
        cout << "TODO: deallocate messages" << endl;
        Panic();
    }
}

unique_ptr<Message> Port::getMessage() {
    assert(lock.isLocked());

    /* only one thread can wait on a port at a given time  */
    if (waitingThread != nullptr) {
        cout << "Tried to get message from Port while another Thread is already waiting" << endl;
        throw BadUserSpace(process);
    }

    if (messages.empty()) {
        return {unique_ptr<Message>()};
    }

    auto msg = move(messages.top());
    messages.pop();
    return move(msg);
}

void Port::setWaitingThread(Thread *thread, size_t index) noexcept {
    assert(lock.isLocked());
    /* only one thread can wait on a port at a given time, TODO: change this */
    assert(waitingThread == nullptr);

    waitingThread = thread;
    waitingThreadIndex = index;
}

void Port::addMessage(unique_ptr<Message> message) {
    Thread *wokenThread;
    {
        scoped_lock sl(lock);
        bool wakeThread = false;

        if (waitingThread) {
            /* A Thread may wait on different ports at a time. If a message on another port is waking
             * up the Thread right now, it may still be referenced in our waitingThread field. We need
             * to esure we only wake it up once. So it will be woken up be the Message the sets the
             * Thread to Transition state first. */
            if (waitingThread->atomicSetState(Thread::State::WaitMessage(),
                                              Thread::State::Transition())) {
                wakeThread = true;
            }
        }

        if (!wakeThread) {
            /* don't wake a Thread, queue message */
            messages.push_back(move(message));
            return;
        }

        /* To wake the Thread, copy message info into it first */
        size_t infoAddress = message->infoAddress.value();

        /* Transfer the waitingThreadIndex variable into the message info */
        waitingThread->process->addressSpace.copyTo(
                    infoAddress,
                    reinterpret_cast<uint8_t *>(&waitingThreadIndex),
                    sizeof(size_t),
                    true);

        waitingThread->kernelStack->userRegisterState()->setSyscallResult(infoAddress);
        wokenThread = waitingThread;
    }

    /* go out of lock scope, so we can update all ports the Thread was waiting on without messing
     * up lock ordering */
    for (auto &port: wokenThread->waitingPorts) {
        scoped_lock sl(port->lock);
        assert(port->waitingThread == wokenThread);
        port->waitingThread = nullptr;
    }

    Scheduler::schedule(wokenThread, true);
}
