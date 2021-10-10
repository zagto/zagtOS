#include <common/common.hpp>
#include <system/System.hpp>
#include <processes/Port.hpp>
#include <processes/Thread.hpp>
#include <processes/Scheduler.hpp>

Port::Port(const shared_ptr<Process> process, Status &) :
    process{process} {}

Port::Port(const hos_v1::Port &handOver,
           const vector<shared_ptr<Thread>> &allThreads,
           Status &status) {
    status = Status::OK();
    if (handOver.threadWaits) {
        waitingThread = allThreads[handOver.waitingThreadID].get();
    }
    for (size_t index = 0; index < handOver.numMessages; index++) {
        auto msg = make_unique<Message>(handOver.messages[index]);
        if (!msg) {
            status = msg.status();
            messages.clear();
            return;
        }
        status = messages.push_back(move(*msg));
        if (!status) {
            return;
        }
    }
}

Port::~Port() {
    assert(waitingThread == nullptr);
    if (!messages.empty()) {
        cout << "TODO: deallocate messages" << endl;
        Panic();
    }
}

Result<unique_ptr<Message>> Port::getMessage() {
    assert(lock.isLocked());

    /* only one thread can wait on a port at a given time  */
    if (waitingThread != nullptr) {
        cout << "Tried to get message from Port while another Thread is already waiting" << endl;
        return Status::BadUserSpace();
    }

    if (messages.empty()) {
        return {unique_ptr<Message>()};
    }

    auto msg = move(messages.top());
    messages.pop();
    return move(msg);
}

void Port::setWaitingThread(Thread *thread) {
    assert(lock.isLocked());
    /* only one thread can wait on a port at a given time */
    assert(waitingThread == nullptr);

    waitingThread = thread;
}

Status Port::addMessage(unique_ptr<Message> message) {
    assert(lock.isLocked());

    if (waitingThread) {
        waitingThread->kernelStack->userRegisterState()->setSyscallResult(message->infoAddress.value());
        waitingThread->setState(Thread::State::Transition());
        Thread *tmp = waitingThread;
        waitingThread = nullptr;
        Scheduler::schedule(tmp, true);
        return Status::OK();
    } else {
        return messages.push_back(move(message));
    }
}
