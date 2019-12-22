#include <common/common.hpp>
#include <system/System.hpp>
#include <processes/Port.hpp>

Port::Port(Thread &thread, vector<shared_ptr<Tag>> &acceptedTags) :
    thread{thread},
    threadWaits{false},
    acceptedTags{move(acceptedTags)} {}

unique_ptr<Message> Port::getMessageOrMakeThreadWait() {
    assert(!threadWaits);
    lock_guard lg(lock);

    if (messages.empty()) {
        CurrentProcessor->scheduler.remove(&thread);
        return nullptr;
    } else {
        auto msg = move(messages.top());
        messages.pop();
        return msg;
    }
}
