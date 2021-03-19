#pragma once

#include <common/common.hpp>
#include <processes/Process.hpp>

enum class USOOperation {
    READ, WRITE, READ_AND_WRITE
};

template <typename T, USOOperation op> class UserSpaceObject {
private:
    size_t address;
    bool valid;

public:
    T object;

    UserSpaceObject(size_t address, Status &status) :
            address{address} {
        if (op != USOOperation::WRITE) {
            status = CurrentProcess()->copyFromUser(reinterpret_cast<uint8_t *>(&object),
                                                    address,
                                                    sizeof(T),
                                                    false);
            valid = status;
        } else {
            valid = true;
        }
    }

    /* constructor for write-only USOs which does not require status checking */
    UserSpaceObject(size_t address) :
        address{address},
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
