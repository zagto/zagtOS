#include <processes/Thread.hpp>
#include <processes/Port.hpp>
#include <processes/Process.hpp>

bool Thread::lookupOwnPort(uint32_t handle, shared_ptr<Port> &port) const {
    if (!process->handleManager.lookup(handle, port)) {
        cout << "lookupOwnPort: invalid port ID" << endl;
        return false;
    }
    if (!port) {
        cout << "lookupOwnPort: port was already destroyed" << endl;
        return false;
    }
    if (!port->ownedBy(*this)) {
        cout << "lookupOwnPort: Port not owned by this thread" << endl;
        return false;
    }
    return true;
}
