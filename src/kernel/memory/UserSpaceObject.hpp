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

    UserSpaceObject(size_t address, Status &status) :
            address{address},
            process{process} {
        if (op != USOOperation::WRITE) {
            status = CurrentProcess()->copyFromUser(reinterpret_cast<uint8_t *>(&object),
                                                    address,
                                                    sizeof(T),
                                                    false);
        }
        valid = status;
    }

    /* constructor for write-only USOs which does not require status checking */
    UserSpaceObject(size_t address, const shared_ptr<Process> &process) :
        address{address},
        process{process},
        valid{true} {

        static_assert(op == USOOperation::WRITE, "readable USOs require status-checking on constructor");
    }

    Status writeOut() {
        static_assert(op != USOOperation::READ, "read-only USOs cannot be written out");
        assert(valid);
        return CurrentProcess()->copyToUser(address, reinterpret_cast<uint8_t *>(&object),
                                            sizeof(T),
                                            true);
    }
};
