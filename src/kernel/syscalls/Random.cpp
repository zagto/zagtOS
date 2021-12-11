#include <syscalls/Random.hpp>

size_t Random(const shared_ptr<Process> &process,
                      size_t address,
                      size_t length,
                      size_t,
                      size_t,
                      size_t) {
    /* TODO: actually generate something random */
    vector<uint8_t> randomValues(length, 0x42);
    process->addressSpace.copyTo(address, randomValues.data(), length, true);
    return 0;
}

