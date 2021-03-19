#include <common/common.hpp>
#include <system/System.hpp>
#include <processes/Port.hpp>

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

unique_ptr<Message> Port::getMessageOrMakeThreadWait(Thread *thread) {
    /* only one thread can wait on a port at a given time */
    assert(waitingThread == nullptr);

    scoped_lock sl(lock);

    if (messages.empty()) {
        thread->currentProcessor()->scheduler.remove(thread);
        thread->setState(Thread::State::WaitMessage(this));
        waitingThread = thread;
        return nullptr;
    } else {
        auto msg = move(messages.top());
        messages.pop();
        return msg;
    }
}

Status Port::addMessage(unique_ptr<Message> message) {
    scoped_lock sl(lock);

    if (waitingThread) {
        //waitingThread->registerState.setSyscallResult(message->infoAddress.value());
        cout << "TODO: implement kernel threading" << endl;
        Panic();
        waitingThread->setState(Thread::State::Transition());
        Thread *tmp = waitingThread;
        waitingThread = nullptr;
        Scheduler::schedule(tmp);
        return Status::OK();
    } else {
        return messages.push_back(move(message));
    }
}
