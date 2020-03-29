#include <mutex>
#include <processes/Process.hpp>
#include <processes/Thread.hpp>
#include <syscalls/SpawnProcess.hpp>
#include <memory/UserSpaceObject.hpp>

bool SpawnProcess::perform(Process &process) {
    /* TODO: permissions checking */

    /* make all the returns in the error handlers return 0 */
    result = 0;

    if (priority >= Thread::NUM_PRIORITIES) {
        cout << "SYS_SPAWN_PROCESS: invalid priority\n";
        return true;
    }

    /* TODO: handle out of memory */
    vector<uint8_t> buffer(length);
    bool valid = process.copyFromUser(&buffer[0], address, length, false);
    if (!valid) {
        cout << "SYS_SPAWN_PROCESS: invalid buffer\n";
        return true;
    }

    ELF elf{Slice<vector, uint8_t>(&buffer)};
    if (!valid) {
        cout << "SYS_SPAWN_PROCESS: invalid ELF\n";
        return true;
    }

    if (numMessageHandles * Message::HANDLE_SIZE > messageSize) {
        cout << "SYS_SPAWN_PROCESS: invalid number of handles: " << numMessageHandles
             << "in message of size " << messageSize << endl;
        return true;
    }

    valid = process.verifyUserAccess(messageAddress, messageSize, false);
    if (!valid) {
        cout << "SYS_SPAWN_PROCESS: message memory not accessible\n";
        return true;
    }

    Message runMessage(&process, nullptr, messageAddress, messageType, messageSize, numMessageHandles);
    new Process(elf, static_cast<Thread::Priority>(priority), runMessage);

    result = 1;
    return true;
}
