#pragma once
#include <processes/UserApi.hpp>
#include <memory>

class Process;

struct Event {
    userApi::ZoEventInfo eventInfo;
    Event(size_t type, size_t tag) :
        eventInfo{type, tag, {}} {}
    Event(userApi::ZoEventInfo eventInfo) :
        eventInfo{eventInfo} {}
    void writeEventInfo(shared_ptr<Process> process, size_t userAddress);
};
