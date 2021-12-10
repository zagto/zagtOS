#include <zagtos/UUID.hpp>
//#include <format>
#include <iostream>

namespace zagtos {

std::ostream &operator<<(std::ostream &stream, UUID uuid) {
    /*return stream << std::format(
                         "{:02x}{:02x}{:02x}{:02x}-{:02x}{:02x}-{:02x}{:02x}-{:02x}{:02x}-{:02x}{:02x}{:02x}{:02x}{:02x}{:02x}",
                         uuid.data[0], uuid.data[1], uuid.data[2], uuid.data[3],
                         uuid.data[4], uuid.data[5], uuid.data[6], uuid.data[7],
                         uuid.data[8], uuid.data[9], uuid.data[10], uuid.data[11],
                         uuid.data[12], uuid.data[13], uuid.data[14], uuid.data[15]);*/
    return stream << std::hex << reinterpret_cast<uint64_t *>(&uuid.data)[0] << "-" << reinterpret_cast<uint64_t *>(&uuid.data)[1];
}

}
