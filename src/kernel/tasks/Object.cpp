#include <common/common.hpp>
#include <tasks/Object.hpp>

size_t Object::sizeInMemory() {
    return info.numDataBytes + sizeof(Object);
}
