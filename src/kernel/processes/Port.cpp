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
    assert(lock.isLocked());

    if (waitingThread) {
        size_t infoAddress = message->infoAddress.value();

        /* Transfer the waitingThreadIndex variable into the message info */
        waitingThread->process->addressSpace.copyTo(
                    infoAddress,
                    reinterpret_cast<uint8_t *>(&waitingThreadIndex),
                    sizeof(size_t),
                    true);

        waitingThread->kernelStack->userRegisterState()->setSyscallResult(infoAddress);
        waitingThread->setState(Thread::State::Transition());
        Thread *tmp = waitingThread;
        waitingThread = nullptr;
        Scheduler::schedule(tmp, true);
    } else {
        return messages.push_back(move(message));
    }
}
