#pragma once

#include <common/common.hpp>
#include <processes/Process.hpp>

enum class USOOperation {
    READ, WRITE, READ_AND_WRITE
};

template <typename T, USOOperation op> class UserSpaceObject {
private:
    size_t address;

public:
    T object;

    UserSpaceObject(size_t address) :
            address{address} {
        if (op != USOOperation::WRITE) {
            CurrentProcess()->addressSpace.copyFrom(reinterpret_cast<uint8_t *>(&object),
                                                    address,
                                                    sizeof(T));
        }
    }

    void writeOut() {
        static_assert(op != USOOperation::READ, "read-only USOs cannot be written out");
        CurrentProcess()->addressSpace.copyTo(address,
                                              reinterpret_cast<const uint8_t *>(&object),
                                              sizeof(T),
                                              true);
    }
};
