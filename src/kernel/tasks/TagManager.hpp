#ifndef TAGMANAGER_HPP
#define TAGMANAGER_HPP

#include <common/inttypes.hpp>

static const size_t MAX_NUM_TAGS = 0x1000 * 16 - 1;
static const uint16_t TAG_USED = MAX_NUM_TAGS;

class TagManager {
private:
    uint16_t *data;
    uint16_t nextFreeTag;

public:
    TagManager();
    ~TagManager();

    uint32_t allocateTag();
    void freeTag(uint32_t tag);
};

#endif // TAGMANAGER_HPP
