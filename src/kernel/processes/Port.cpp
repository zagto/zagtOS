#include <common/common.hpp>
#include <system/System.hpp>
#include <processes/Port.hpp>
#include <processes/Thread.hpp>
#include <processes/Scheduler.hpp>
#include <system/Processor.hpp>

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

Result<unique_ptr<Message>> Port::getMessageOrMakeThreadWait() {
    /* only one thread can wait on a port at a given time */
    assert(waitingThread == nullptr);

    scoped_lock sl(lock);
    if (messages.empty()) {
        /* Danger Zone: Asymmetric lock/unlock: These two locks will not be unlocked once this
         * method leaves, but once the thread state is discarded. */
        Processor::kernelInterruptsLock.lock();
        CurrentProcessor->scheduler.lock.lock();

        Thread *thread = CurrentThread();
        CurrentProcessor->scheduler.removeActiveThread();
        thread->setState(Thread::State::WaitMessage(this));

        waitingThread = thread;

        return Status::DiscardStateAndSchedule();
    } else {
        auto msg = move(messages.top());
        messages.pop();
        lock.unlock();
        return move(msg);
    }
}

Status Port::addMessage(unique_ptr<Message> message) {
    scoped_lock sl(lock);

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
