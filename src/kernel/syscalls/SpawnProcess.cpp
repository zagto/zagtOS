#include <mutex>
#include <processes/Process.hpp>
#include <processes/Thread.hpp>
#include <syscalls/SpawnProcess.hpp>
#include <memory/UserSpaceObject.hpp>

bool SpawnProcess::perform(const shared_ptr<Process> &process) {
    /* TODO: permissions checking */

    /* make all the returns in the error handlers return 0 */
    result = 0;

    if (priority >= Thread::NUM_PRIORITIES) {
        cout << "SYS_SPAWN_PROCESS: invalid priority\n";
        return true;
    }

    if (numSections < 1) {
        cout << "SYS_SPAWN_PROCESS: tried to spawn process with 0 sections\n";
        return true;
    }

    /* TODO: handle out of memory */
    vector<SpawnProcessSection> sections(numSections);
    bool valid = process->copyFromUser(reinterpret_cast<uint8_t *>(&sections[0]),
                                       sectionsAddress,
                                       numSections * sizeof(SpawnProcessSection),
                                       false);
    if (!valid) {
        cout << "SYS_SPAWN_PROCESS: invalid sections info buffer\n";
        return true;
    }

    vector<uint8_t> logNameBuffer(logNameSize);
    valid = process->copyFromUser(&logNameBuffer[0], logNameAddress, logNameSize, false);
    if (!valid) {
        cout << "SYS_SPAWN_PROCESS: invalid log name buffer\n";
        return true;
    }

    for (size_t index = 0; index < numSections; index++) {
        SpawnProcessSection section = sections[index];

        if (section.address + section.sizeInMemory < section.address) {
            cout << "SYS_SPAWN_PROCESS: Integer Overflow in section target address + size" << endl;
            return true;
        }

        if (!section.region().isPageAligned()
                || section.dataSize % PAGE_SIZE != 0) {
            cout << "SYS_SPAWN_PROCESS: section region is not page-aligned" << endl;
        }

        static_assert(UserSpaceRegion.start == 0, "This code assumes user space starts at 0");
        if (!VirtualAddress::checkInRegion(UserSpaceRegion, section.region().end())) {
            cout << "SYS_SPAWN_PROCESS: segment does not fit in user space" << endl;
            return true;
        }

        if ((section.flags & section.FLAG_WRITEABLE)
                && (section.flags & section.FLAG_EXECUTABLE)) {
            cout << "SYS_SPAWN_PROCESS: Segment is marked as writeable and executable at the same time"
                << endl;
            return true;
        }

        for (size_t otherIndex = index + 1; otherIndex < numSections; otherIndex++) {
            SpawnProcessSection otherSection = sections[otherIndex];

            if (section.region().overlaps(otherSection.region())) {
                cout << "SYS_SPAWN_PROCESS: Segments overlap" << section.address << ":"
                     << section.sizeInMemory << ", "
                     << otherSection.address << ":"
                     << otherSection.sizeInMemory << endl;
                return true;
            }
        }
    }

    optional<SpawnProcessSection> TLSSection;
    if (TLSSectionAddress != 0) {
        SpawnProcessSection section;
        bool valid = process->copyFromUser(reinterpret_cast<uint8_t *>(&section),
                                           TLSSectionAddress,
                                           sizeof(SpawnProcessSection),
                                           false);
        if (!valid) {
            cout << "SYS_SPAWN_PROCESS: invalid TLS sections info buffer\n";
            return true;
        }

        if (section.sizeInMemory % PAGE_SIZE != 0
                || section.dataSize % PAGE_SIZE != 0) {
            cout << "SYS_SPAWN_PROCESS: TLS region is not page-aligned" << endl;
        }

        if ((section.flags & section.FLAG_WRITEABLE)
                && (section.flags & section.FLAG_EXECUTABLE)) {
            cout << "ELF: Segment is marked as writeable and executable at the same time"
                << endl;
            return true;
        }
        TLSSection = section;
    }

    if (!process->verifyMessageAccess(messageAddress, messageSize, numMessageHandles)) {
        return true;
    }

    Message runMessage(process.get(),
                       nullptr,
                       messageAddress,
                       messageType,
                       messageSize,
                       numMessageHandles);
    new Process(*process,
                sections,
                TLSSection,
                entryAddress,
                static_cast<Thread::Priority>(priority),
                runMessage,
                move(logNameBuffer));

    result = 1;
    return true;
}

Permissions SpawnProcessSection::permissions() const {
    if (flags & FLAG_EXECUTABLE) {
        return Permissions::READ_EXECUTE;
    } else if (flags & FLAG_WRITEABLE) {
        return Permissions::READ_WRITE;
    } else {
        return Permissions::READ;
    }
}

Region SpawnProcessSection::region() const {
    return {address, sizeInMemory};
}
