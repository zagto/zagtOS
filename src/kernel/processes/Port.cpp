#include <common/common.hpp>
#include <system/System.hpp>
#include <processes/Port.hpp>

Port::Port(Process &process) :
    process{process} {}

unique_ptr<Message> Port::getMessageOrMakeThreadWait(Thread *thread) {
    /* only one thread can wait on a port at a given time */
    assert(waitingThread == nullptr);

    lock_guard lg(lock);

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
