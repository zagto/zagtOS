#include <processes/Thread.hpp>
#include <processes/Port.hpp>
#include <processes/Process.hpp>

bool Process::verifyMessageAccess(size_t address, size_t length, size_t numHandles) {
    assert(pagingLock.isLocked());

    if (numHandles * Message::HANDLE_SIZE > length) {
        cout << "verifyMessageAccess: invalid number of handles: " << numHandles
             << "in message of size " << length << endl;
        return false;
    }

    bool valid = verifyUserAccess(address, length, false);
    if (!valid) {
        cout << "verifyMessageAccess: message memory not accessible\n";
        return false;
    }
    return true;
}
