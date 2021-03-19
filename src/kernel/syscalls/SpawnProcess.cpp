#include <mutex>
#include <processes/Thread.hpp>
#include <syscalls/SpawnProcess.hpp>
#include <syscalls/ErrorCodes.hpp>
#include <syscalls/UserSpaceObject.hpp>

Result<size_t> SpawnProcess(const shared_ptr<Process> &process,
                            uint64_t structAddress,
                            uint64_t,
                            uint64_t,
                            uint64_t,
                            uint64_t) {
    Status status;
    UserSpaceObject<SpawnProcessStruct, USOOperation::READ> uso(structAddress, status);
    if (!status) {
        return status;
    }
    return uso.object.perform(process);
}

Result<size_t> SpawnProcessStruct::perform(const shared_ptr<Process> &process) {
    /* TODO: permissions checking */

    /* make all the some returns in the error handlers return EINVAL to user space.
     * return OK as kernel status so the process continues running.
     * Processes should be able to handle broken SpawnProcesses gracefully
     * since processes like UserEnvironment they may be launching processes
     * on behalf of others which are not fully trusted */

    if (priority >= Thread::NUM_PRIORITIES) {
        cout << "SYS_SPAWN_PROCESS: invalid priority\n";
        return Status::BadUserSpace();
    }

    if (numSections < 1) {
        cout << "SYS_SPAWN_PROCESS: tried to spawn process with 0 sections\n";
        return EINVAL;
    }

    Status status = Status::OK();
    vector<SpawnProcessSection> sections(numSections, status);
    if (!status) {
        return status;
    }
    status = process->copyFromUser(reinterpret_cast<uint8_t *>(&sections[0]),
                                   sectionsAddress,
                                   numSections * sizeof(SpawnProcessSection),
                                   false);
    if (!status) {
        if (status == Status::BadUserSpace()) {
            cout << "SYS_SPAWN_PROCESS: invalid sections info buffer\n";
        }
        return status;
    }

    vector<uint8_t> logNameBuffer(logNameSize, status);
    if (!status) {
        return status;
    }
    status = process->copyFromUser(logNameBuffer.data(), logNameAddress, logNameSize, false);
    if (!status) {
        if (status == Status::BadUserSpace()) {
            cout << "SYS_SPAWN_PROCESS: invalid log name buffer\n";
        }
        return status;
    }

    for (size_t index = 0; index < numSections; index++) {
        SpawnProcessSection section = sections[index];

        if (section.address + section.sizeInMemory < section.address) {
            cout << "SYS_SPAWN_PROCESS: Integer Overflow in section target address + size" << endl;
            return EINVAL;
        }

        if (!section.region().isPageAligned()
                || section.dataSize % PAGE_SIZE != 0) {
            cout << "SYS_SPAWN_PROCESS: section region is not page-aligned" << endl;
            return EINVAL;
        }

        static_assert(UserSpaceRegion.start == 0, "This code assumes user space starts at 0");
        if (!VirtualAddress::checkInRegion(UserSpaceRegion, section.region().end())) {
            cout << "SYS_SPAWN_PROCESS: segment does not fit in user space" << endl;
            return EINVAL;
        }

        if ((section.flags & section.FLAG_WRITEABLE)
                && (section.flags & section.FLAG_EXECUTABLE)) {
            cout << "SYS_SPAWN_PROCESS: Segment is marked as writeable and executable at the same time"
                << endl;
            return EINVAL;
        }

        for (size_t otherIndex = index + 1; otherIndex < numSections; otherIndex++) {
            SpawnProcessSection otherSection = sections[otherIndex];

            if (section.region().overlaps(otherSection.region())) {
                cout << "SYS_SPAWN_PROCESS: Segments overlap" << section.address << ":"
                     << section.sizeInMemory << ", "
                     << otherSection.address << ":"
                     << otherSection.sizeInMemory << endl;
                return EINVAL;
            }
        }
    }

    optional<SpawnProcessSection> TLSSection;
    if (TLSSectionAddress != 0) {
        SpawnProcessSection section;
        status = process->copyFromUser(reinterpret_cast<uint8_t *>(&section),
                                           TLSSectionAddress,
                                           sizeof(SpawnProcessSection),
                                           false);
        if (!status) {
            if (status == Status::BadUserSpace()) {
                cout << "SYS_SPAWN_PROCESS: invalid TLS sections info buffer\n";
            }
            return status;
        }

        if (section.sizeInMemory % PAGE_SIZE != 0
                || section.dataSize % PAGE_SIZE != 0) {
            cout << "SYS_SPAWN_PROCESS: TLS region is not page-aligned" << endl;
        }

        if ((section.flags & section.FLAG_WRITEABLE)
                && (section.flags & section.FLAG_EXECUTABLE)) {
            cout << "ELF: Segment is marked as writeable and executable at the same time"
                << endl;
            return EINVAL;
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
                       numMessageHandles,
                       status);
    if (!status) {
        return status;
    }

    Result newProcess = make_raw<Process>(*process,
                        sections,
                        TLSSection,
                        entryAddress,
                        static_cast<Thread::Priority>(priority),
                        runMessage,
                        logNameBuffer);
    if (newProcess) {
        return 0;
    } else {
        return newProcess.status();
    }
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
