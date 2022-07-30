#pragma once

#include <mutex>
#include <queue>

class Timer {
private:
    struct Event {
        enum Type {
            WAKE_THREAD
        };

        size_t time;
        Thread *thread;
    };

    mutex lock;
    queue<Event> events;

public:

};
