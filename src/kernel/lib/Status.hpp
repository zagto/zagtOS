#pragma once

#include <common/panic.hpp>
#include <common/utils.hpp>

enum StatusType {
    OK, OutOfMemory, OutOfKernelHeap, BadUserSpace, ThreadKilled, NonInitialized, DiscardStateAndSchedule
};

template<typename T> class shared_ptr;
class Process;
class Logger;

#ifdef __cplusplus
class [[nodiscard("Status code ignored")]] Status  {
private:
    StatusType _type{StatusType::NonInitialized};
    size_t data[2]{0};


    friend Logger &operator<<(Logger &logger, Status &status);

public:
    Status() {}
    Status(StatusType type) :
        _type{type},
        data{0} {}

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
    static Status DiscardStateAndSchedule() {
        return Status(StatusType::DiscardStateAndSchedule);
    }
    StatusType type() const {
        return _type;
    }
    explicit operator bool() const {
        assert(_type != StatusType::NonInitialized);
        return _type == StatusType::OK;
    }
    bool operator==(const Status &other) {
        return _type == other._type;
    }
    bool operator!=(const Status &other) {
        return _type != other._type;
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
        assert(!static_cast<bool>(_status));
    }
    T *operator->() {
        assert(static_cast<bool>(_status));
        return &_value;
    }
    T &operator*() {
        assert(static_cast<bool>(_status));
        return _value;
    }
    explicit operator bool() {
        return static_cast<bool>(_status);
    }
    Status status() {
        Status result = _status;
        return result;
    }
};
#endif
