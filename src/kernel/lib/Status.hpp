#pragma once

#include <common/panic.hpp>
#include <common/utils.hpp>

enum StatusType {
    OK, OutOfMemory, OutOfKernelHeap, BadUserSpace, NonInitialized
};

class Process;

#ifdef __cplusplus
class [[nodiscard("Status code ignored")]] Status  {
private:
    StatusType type{StatusType::NonInitialized};
    bool handled{false};

    [[noreturn]] void unhandled();

public:
    Status() {}
    Status(StatusType type) :
        type{type} {}
    ~Status() {
        if (type != StatusType::NonInitialized && !handled) {
            unhandled();
        }
    }
    static Status OK() {
        return Status(StatusType::OK);
    }
    static Status BadUserSpace() {
        return Status(StatusType::BadUserSpace);
    }
    static Status OutOfKernelHeap(/*Process &initiator, size_t allocationSize*/) {
        return  Status(StatusType::OutOfKernelHeap);
    }
    static Status OutOfMemory() {
        return Status(Status::OutOfMemory());
    }
    operator bool() {
        assert(type != StatusType::NonInitialized);
        handled = true;
        return type == StatusType::OK;
    }
    void setHandled() {
        assert(type != StatusType::NonInitialized);
        handled = true;
    }
};

template<typename T> class [[nodiscard("Result value ignored")]] Result {
private:
    Status _status;
    T _value;

public:
    constexpr Result() {}
    Result(T value):
        _status{Status::OK()},
        _value{move(value)} {}
    Result(Status status):
            _status{status} {
        assert(!_status);
    }
    T *operator->() {
        assert(_status);
        return &_value;
    }
    T &operator*() {
        assert(_status);
        return _value;
    }
    operator bool() {
        return _status;
    }
    Status status() {
        Status result = _status;
        _status.setHandled();
        return result;
    }
};
#endif
