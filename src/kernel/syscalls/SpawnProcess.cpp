#include <lib/Lock.hpp>
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

    valid = process.verifyUserAccess(messageAddress, messageSize, false);
    if (!valid) {
        cout << "SYS_SPAWN_PROCESS: invalid message\n";
        return true;
    }

    Process *newProcess = new Process(elf,
                             static_cast<Thread::Priority>(priority),
                             messageType,
                             messageSize);
    LockHolder lh2(newProcess->pagingLock);
    valid = newProcess->copyFromOhterUserSpace(newProcess->runMessageAddress(),
                                            &process,
                                            messageAddress,
                                            messageSize,
                                            false);
    /* the checks before should have caught everything */
    assert(valid);

    result = 1;
    return true;
}
