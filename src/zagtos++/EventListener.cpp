#include <zagtos/EventListener.hpp>

namespace zagtos {

void DefaultEventLoop(zagtos::EventQueue& queue) {
    while (true) {
        zagtos::Event event = queue.waitForEvent();
        auto listener = reinterpret_cast<zagtos::EventListener *>(event.tag());
        listener->handleEvent(event);
    }
}

}
