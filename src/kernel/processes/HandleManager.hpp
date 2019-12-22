#pragma once

#include <common/inttypes.hpp>
#include <vector>
#include <memory>
#include <mutex>
#include <processes/Port.hpp>
#include <processes/Thread.hpp>

namespace handleManager {

enum class Type : uint32_t {
    INVALID, FREE, PORT, TAG, THREAD
};

template<typename T>
static Type typeIdOf();
template<>
Type typeIdOf<Port>() {
    return Type::PORT;
}
template<>
Type typeIdOf<Tag>() {
    return Type::TAG;
}
template<>
Type typeIdOf<Thread>() {
    return Type::THREAD;
}


struct alignas(weak_ptr<Port>) Handle {
    uint8_t data[sizeof(weak_ptr<Port>)];
    Type type;

    Handle():
        type{Type::INVALID} {}
    template<typename T>
    Handle(shared_ptr<T> element) {
        type = typeIdOf<T>();
        new (data) shared_ptr<T>(move(element));
    }
    ~Handle() {
        switch (type) {
        case Type::PORT:
            removeValue<Port>(0);
            return;
        case Type::TAG:
            removeValue<Tag>(0);
            return;
        case Type::THREAD:
            removeValue<Thread>(0);
            return;
        default:
            ; /* do nothing */
        }
    }
    Handle(Handle &&other) {
        type = other.type;
        memcpy(data, other.data, sizeof(data));
        other.type = Type::INVALID;
    }
    void operator=(Handle &) = delete;
    void operator=(Handle &&other) {
        assert(type == Type::FREE || type == Type::INVALID);
        type = other.type;
        memcpy(data, other.data, sizeof(data));
        other.type = Type::INVALID;
    }
    template<typename T>
    void removeValue(uint32_t nextFreeHandle) {
        assert(type == typeIdOf<T>());
        reinterpret_cast<T *>(data)->~T();
        *reinterpret_cast<uint32_t *>(data) = nextFreeHandle;
    }
    uint32_t getNextFree() {
        assert(type == Type::FREE);
        return *reinterpret_cast<uint32_t *>(data);
    }
};

class HandleManager {
public:
    static const uint32_t HANDLE_END = static_cast<uint32_t>(-1);

private:
    vector<Handle> handles;
    uint32_t nextFreeHandle{HANDLE_END};

public:
    HandleManager() {}
    HandleManager(HandleManager &) = delete;

    /* returns false if given handle is bogus
     * returns true and sets result to a shared_ptr, which may be empty if the object the handle was
     * referring to no longer exists */
    template<typename T> bool lookup(uint32_t handle, shared_ptr<T> &result) {
        if (handle >= handles.size() || handles[handle].type != typeIdOf<T>()) {
            return false;
        }
        weak_ptr<T> *weakPointer = reinterpret_cast<weak_ptr<T> *>(handles[handle].data);
        result = weakPointer->lock();
        return true;
    }
    template<typename T> uint32_t add(shared_ptr<T> newElement) {
        if (handles.size() == HANDLE_END && nextFreeHandle == HANDLE_END) {
            return HANDLE_END;
        }
        if (nextFreeHandle == HANDLE_END) {
            handles.push_back(move(Handle(newElement)));
            return static_cast<uint32_t>(handles.size()) - 1;
        } else {
            uint32_t handle = nextFreeHandle;
            nextFreeHandle = handles[handle].getNextFree();
            handles[handle] = Handle(newElement);
            return handle;
        }
    }
};

}

using handleManager::HandleManager;

