#pragma once

#include <common/common.hpp>
#include <processes/Process.hpp>

enum class USOOperation {
    READ, WRITE, READ_AND_WRITE
};

template <typename T, USOOperation op> class UserSpaceObject {
private:
    bool valid;
    size_t address;
    const shared_ptr<Process> &process;

public:
    T object;

    UserSpaceObject(size_t address, const shared_ptr<Process> &process, Status &status) :
            address{address},
            process{process} {
        if (op == USOOperation::WRITE) {
            status = process->verifyUserAccess(address, sizeof(T), true);
        } else {
            status = process->copyFromUser(reinterpret_cast<uint8_t *>(&object),
                                           address,
                                           sizeof(T),
                                           /* already check for write permissions if we need them
                                            * later */
                                           op == USOOperation::READ_AND_WRITE);
        }
        valid = status;
    }

    Status writeOut() {
        assert(op != USOOperation::READ);
        assert(valid);
        return process->copyToUser(address, reinterpret_cast<uint8_t *>(&object), sizeof(T), true);
    }
};
