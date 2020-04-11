#include <common/common.hpp>
#include <system/System.hpp>
#include <processes/Port.hpp>

Port::Port(const shared_ptr<Process> process) :
    process{process} {}

unique_ptr<Message> Port::getMessageOrMakeThreadWait(Thread *thread) {
    /* only one thread can wait on a port at a given time */
    assert(waitingThread == nullptr);

    scoped_lock sl(lock);

    if (messages.empty()) {
        CurrentProcessor->scheduler.remove(thread);
        waitingThread = thread;
        return nullptr;
    } else {
        auto msg = move(messages.top());
        messages.pop();
        return msg;
    }
}

void Port::addMessage(unique_ptr<Message> message) {
    scoped_lock sl(lock);

    if (waitingThread) {
        cout << "waking thread wainting for message. TODO: processor assignment" << endl;

        waitingThread->registerState.setSyscallResult(message->infoAddress().value());
        CurrentProcessor->scheduler.add(waitingThread);
        waitingThread = nullptr;
    } else {
        messages.push_back(move(message));
    }
}
