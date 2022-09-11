#include <zagtos/EventQueue.hpp>
#include <zagtos/SharedMemory.hpp>
#include <zagtos/Messaging.hpp>
#include <zagtos/syscall.h>

namespace zagtos {

Event::~Event() {
    if (isMessage()) {
        if (info.messageInfo.data.allocatedExternally) {
            UnmapWhole(info.messageInfo.data.data);
        }
    }
}

bool Event::isMessage() const {
    return info.type == cApi::ZAGTOS_EVENT_MESSAGE;
}

bool Event::isInterrupt() const {
    return info.type == cApi::ZAGTOS_EVENT_INTERRUPT;
}

size_t Event::tag() const {
    return info.tag;
}

UUID Event::messageType() const {
    return UUID(info.messageInfo.type);
}

const MessageData &Event::messageData() const {
    return *static_cast<const MessageData *>(&info.messageInfo.data);
}

extern "C" cApi::ZoProcessStartupInfo *__process_startup_info;
EventQueue DefaultEventQueue(__process_startup_info);

EventQueue::EventQueue() {
    _handle = static_cast<uint32_t>(zagtos_syscall0(SYS_CREATE_EVENT_QUEUE));
}

EventQueue::EventQueue(cApi::ZoProcessStartupInfo *startupInfo) {
    _handle = {startupInfo->eventQueueHandle};
}

Event EventQueue::waitForEvent() {
    cApi::ZoEventInfo eventInfo;
    size_t result = zagtos_syscall2(SYS_WAIT_FOR_EVENT, _handle, reinterpret_cast<size_t>(&eventInfo));
    assert(result == 0);
    return Event(eventInfo);
}

}
