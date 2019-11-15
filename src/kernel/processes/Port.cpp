#include <common/common.hpp>
#include <system/System.hpp>
#include <processes/Port.hpp>

Port::Port(Process &process, vector<uint32_t> acceptedTags) :
    process{process},
    acceptedTags{acceptedTags} {

    _id = CurrentSystem.tagManager.allocateTag();
}

Port::~Port() {
    CurrentSystem.tagManager.freeTag(_id);
}

uint32_t Port::id() {
    return _id;
}
