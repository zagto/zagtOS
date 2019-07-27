#ifndef USERSPACEOBJECT_HPP
#define USERSPACEOBJECT_HPP

#include <common/common.hpp>
#include <tasks/Task.hpp>

enum class USOOperation {
    READ, WRITE, READ_AND_WRITE
};

template <typename T, USOOperation op> class UserSpaceObject {
public:
    T object;
    size_t address;
    Task *task;
    bool valid;

    UserSpaceObject(size_t address, Task *task) :
            address{address},
            task{task} {
        if (op == USOOperation::WRITE) {
            valid = task->verifyUserAccess(address, sizeof(T), true);
        } else {
            valid = task->copyFromUser(reinterpret_cast<uint8_t *>(&object),
                                       address,
                                       sizeof(T),
                                       /* already check for write permissions if we need them
                                        * later */
                                       op == USOOperation::READ_AND_WRITE);
        }
    }
    ~UserSpaceObject() {
        if (op != USOOperation::READ) {
            bool result = task->copyToUser(address, reinterpret_cast<uint8_t *>(&object), sizeof(T), true);
            assert(result);
        }
    }
};

#endif // USERSPACEOBJECT_HPP
