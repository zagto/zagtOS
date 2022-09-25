#pragma once

#include <zagtos/HandleObject.hpp>
#include <zagtos/KernelApi.h>
#include <zagtos/UUID.hpp>

namespace zagtos {

struct MessageData;

class Event {
private:
    cApi::ZoEventInfo info;

protected:
    friend class EventQueue;
    Event(cApi::ZoEventInfo info):
        info{info} {}

public:
    ~Event();
    bool isMessage() const;
    bool isInterrupt() const;
    size_t tag() const;
    UUID messageType() const;
    const MessageData &messageData() const;
};

class EventQueue : public HandleObject {
public:
    using HandleObject::HandleObject;
    EventQueue();
    EventQueue(cApi::ZoProcessStartupInfo *startupInfo);
    Event waitForEvent();
};

extern EventQueue DefaultEventQueue;

}
