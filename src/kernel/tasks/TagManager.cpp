#include <common/common.hpp>
#include <tasks/TagManager.hpp>

TagManager::TagManager() {
    data = new uint16_t[MAX_NUM_TAGS];
    for (uint16_t i = 0; i < MAX_NUM_TAGS - 1; i++) {
        data[i] = i + 1;
    }
    data[MAX_NUM_TAGS - 1] = 0;

    nextFreeTag = 0;
}

uint32_t TagManager::allocateTag() {
    assert(data[nextFreeTag != TAG_USED]);
    uint32_t newTag = nextFreeTag;

    nextFreeTag = data[newTag];
    data[newTag] = TAG_USED;
    return newTag;
}

void TagManager::freeTag(uint32_t tag) {
    assert(tag < MAX_NUM_TAGS);
    assert(data[tag] == TAG_USED);

    data[tag] = nextFreeTag;
    nextFreeTag = static_cast<uint16_t>(tag);
}

TagManager::~TagManager() {
    Panic();
}
