#include <syscalls/Random.hpp>

Result<size_t> Random(const shared_ptr<Process> &process,
                      size_t address,
                      size_t length,
                      size_t,
                      size_t,
                      size_t) {
    Status status = Status::OK();
    /* TODO: actually generate something random */
    vector<uint8_t> randomValues(length, 0x42, status);
    if (!status) {
        return status;
    }

    status = process->addressSpace.copyTo(address, randomValues.data(), length, true);
    if (!status) {
        return status;
    }
    return 0;
}

