#include <common/common.hpp>
#include <tasks/Object.hpp>

usize Object::sizeInMemory() {
    return info.numDataBytes + sizeof(Object);
}
