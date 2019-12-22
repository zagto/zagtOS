#include <mutex>
#include <vector>
#include <memory/UserSpaceObject.hpp>
#include <system/System.hpp>
#include <processes/Thread.hpp>
#include <processes/Port.hpp>
#include <syscalls/MappingOperation.hpp>
#include <syscalls/SpawnProcess.hpp>
#include <syscalls/SyscallNumbers.hpp>


bool Thread::handleSyscall() {
    switch (registerState.syscallNr()) {
    case SYS_LOG: {
        lock_guard lg(process->pagingLock);
        static const size_t MAX_LOG_SIZE = 10000;
        size_t address = registerState.syscallParameter(1);
        size_t length = registerState.syscallParameter(0);

        if (length > MAX_LOG_SIZE) {
            cout << "Process attempted to send huge log. ingnoring.\n";
            return true;
        }

        if (length == 0) {
            return true;
        }

        vector<uint8_t> buffer(length);
        bool valid = process->copyFromUser(&buffer[0], address, length, false);
        if (!valid) {
            cout << "SYS_LOG: invalid buffer\n";
            return false;
        }
        for (size_t index = 0; index < buffer.size(); index++) {
            cout << static_cast<char>(buffer[index]);
        }
        return true;
    }
    case SYS_EXIT:
        cout << "Thread Exit" << endl;
        delete this;
        return true;
    case SYS_CREATE_PORT: {
        lock_guard lg(process->pagingLock);
        lock_guard lg2(process->portsLock);
        size_t tagsAddress = registerState.syscallParameter(0);
        size_t numTags = registerState.syscallParameter(1);

        vector<uint32_t> tagHandles(numTags);
        vector<shared_ptr<Tag>> acceptedTags;
        acceptedTags.reserve(numTags);
        if (numTags > 0) {
            bool valid = process->copyFromUser(reinterpret_cast<uint8_t *>(tagHandles.data()),
                                            tagsAddress,
                                            numTags * sizeof(uint32_t),
                                            false);
            if (!valid) {
                return false;
            }
        }

        for (uint32_t handle: tagHandles) {
            shared_ptr<Tag> tag;
            if (!process->handleManager.lookup(handle, tag) || !tag) {
                return false;
            }
            acceptedTags.push_back(tag);
        }

        auto port = make_shared<Port>(*this, acceptedTags);
        uint32_t handle = process->handleManager.add(port);
        if (handle == HandleManager::HANDLE_END) {
            cout << "SYS_CREATE_PORT: out of handles" << endl;
            return false;
        }
        registerState.setSyscallResult(handle);
        port.addGhostReference();

        cout << "created port " << handle << endl;
        return true;
    }
    case SYS_RECEIVE_MESSAGE: {
        lock_guard lg(process->portsLock);
        uint32_t portHandle = static_cast<uint32_t>(registerState.syscallParameter(0));
        shared_ptr<Port> port;
        if (!lookupOwnPort(portHandle, port)) {
            return false;
        }

        unique_ptr<Message> msg = port->getMessageOrMakeThreadWait();
        if (msg) {
            registerState.setSyscallResult(msg->headerAddress.value());
        }
        return true;
    }
    case SYS_DESTROY_PORT: {
        lock_guard lg(process->portsLock);
        uint32_t portHandle = static_cast<uint32_t>(registerState.syscallParameter(0));
        shared_ptr<Port> port;
        if (!lookupOwnPort(portHandle, port)) {
            return false;
        }
        port.removeGhostReference();
        return true;
    }
    case SYS_RANDOM:
        // todo: should write to memory here
        // 0 = pointer,
        // 1 = length
        return true;

    case SYS_MMAP: {
        lock_guard lg(process->pagingLock);
        UserSpaceObject<MMap, USOOperation::READ_AND_WRITE> uso(registerState.syscallParameter(0),
                                                                process);
        if (!uso.valid) {
            return false;
        }
        uso.object.perform(*process);
        return true;
    }
    case SYS_MUNMAP: {
        lock_guard lg(process->pagingLock);
        MUnmap munmap(registerState.syscallParameter(0), registerState.syscallParameter(1));
        munmap.perform(*process);
        registerState.setSyscallResult(munmap.error);
        return true;
    }
    case SYS_GET_ACPI_ROOT: {
        /* TODO: permissions checking */
        registerState.setSyscallResult(CurrentSystem.ACPIRoot.value());
        return true;
    }
    case SYS_ADD_PROCESSOR: {
        /* TODO: permissions checking */
        size_t hardwareID = registerState.syscallParameter(0);
        CurrentProcessor->interrupts.wakeSecondaryProcessor(hardwareID);
        return true;
    }
    case SYS_SPAWN_PROCESS: {
        lock_guard lg(process->pagingLock);
        UserSpaceObject<SpawnProcess, USOOperation::READ_AND_WRITE> uso(registerState.syscallParameter(0),
                                                                    process);
        if (!uso.valid) {
            return false;
        }
        return uso.object.perform(*process);
    }
    default:
        return false;
    }
}
