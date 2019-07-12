#ifndef USERSPACEWINDOW_H
#define USERSPACEWINDOW_H

#include <common/common.hpp>

class UserSpaceWindow {
protected:
    usize _size;
    bool valid;
    u8 *pointer;

public:
    UserSpaceWindow(usize address, usize size, usize alignment = 0);
    u8 &operator[](usize index);
    bool isValid();
    usize size();
};

template <typename T> class UserSpaceObject : public UserSpaceWindow {
public:
    UserSpaceObject(usize address) : UserSpaceWindow(address, sizeof(T), alignof(T)) { }
    T *object() {
        return reinterpret_cast<T *>(pointer);
    }
};

#endif // USERSPACEWINDOW_H
