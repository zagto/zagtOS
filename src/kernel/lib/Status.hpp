#pragma once

#include <common/panic.hpp>
#include <common/utils.hpp>

enum StatusType {
    OK, OutOfMemory, OutOfKernelHeap, BadUserSpace, ThreadKilled, NonInitialized
};

class Process;
class Logger;

#ifdef __cplusplus
class [[nodiscard("Status code ignored")]] Status  {
private:
    StatusType type{StatusType::NonInitialized};
    friend Logger &operator<<(Logger &logger, Status &status);

public:
    Status() {}
    Status(StatusType type) :
        type{type} {}
    static Status OK() {
        return Status(StatusType::OK);
    }
    static Status BadUserSpace() {
        return Status(StatusType::BadUserSpace);
    }
    static Status OutOfKernelHeap(/*Process &initiator, size_t allocationSize*/) {
        return  Status(StatusType::OutOfKernelHeap);
    }
    static Status OutOfMemory() { /* how much, zone id */
        return Status(StatusType::OutOfMemory);
    }
    static Status ThreadKilled() {
        return Status(StatusType::ThreadKilled);
    }
    operator bool() const {
        assert(type != StatusType::NonInitialized);
        return type == StatusType::OK;
    }
    bool operator==(const Status &other) {
        return type == other.type;
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
        return result;
    }
};
#endif
