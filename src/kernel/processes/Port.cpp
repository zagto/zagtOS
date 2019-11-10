#include <common/common.hpp>
#include <system/System.hpp>
#include <processes/Port.hpp>

Port::Port(vector<uint32_t> acceptedTags) :
    acceptedTags{acceptedTags} {

    _id = CurrentSystem.tagManager.allocateTag();
}

Port::~Port() {
    CurrentSystem.tagManager.freeTag(_id);
}

uint32_t Port::id() {
    return _id;
}
