#pragma once

#include <zagtos/HandleObject.hpp>

namespace zagtos {

class IOPortRange : public HandleObject {
public:
    IOPortRange() {}
    IOPortRange(uint16_t start, uint16_t length);
    IOPortRange(IOPortRange &) = delete;
    IOPortRange(IOPortRange &&other) : HandleObject(std::move(other)) {}
    IOPortRange &operator=(IOPortRange &other) = delete;
    IOPortRange &operator=(IOPortRange &&other);

    uint32_t read(uint16_t offset, size_t size);
    void write(uint16_t offset, size_t length, uint32_t size);
};

}
