#pragma once

#include <common/panic.hpp>
#include <common/utils.hpp>

enum StatusType {
    OK, OutOfMemory, OutOfKernelHeap, BadUserSpace
};

class Process;

#ifdef __cplusplus
class [[nodiscard("Status code ignored")]] Status  {
private:
    StatusType type;
    bool handled{false};

    [[noreturn]] void unhandled();

public:
    Status() {}
    Status(StatusType type) :
        type{type} {}
    ~Status() {
        if (!handled) {
            unhandled();
        }
    }
    static Status OK() {
        return Status(StatusType::OK);
    }
    static Status BadUserSpace() {
        return Status(StatusType::BadUserSpace);
    }
    static Status OutOfKernelHeap(/*Process &initiator, size_t allocationSize*/);
    operator bool() const {
        return type == StatusType::OK;
    }
};

template<typename T> class [[nodiscard("Result value ignored")]] Result {
private:
    Status _status;
    T _value;

public:
    constexpr Result() {}
    Result(T value):
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
    operator bool() const {
        return _status;
    }
    Status status() const {
        return _status;
    }
};
#endif
