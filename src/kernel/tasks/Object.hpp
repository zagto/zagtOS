#ifndef OBJECT_HPP
#define OBJECT_HPP

#include <common/common.hpp>

class Object;

class UUID {
    private:
        uint64_t data[2];

    public:
        UUID() :
            data{0, 0} {}

        UUID(uint64_t part1, uint64_t part2) :
            data{part1, part2} {}

        UUID(const UUID &other) :
            data{other.data[0], other.data[1]} {}
};

static const UUID INIT_MSG{0x0ad8be825784530, 0x835ab01bf8435fd2};


class ObjectInfo {
    protected:
        friend class Object;
        UUID id;
        UUID type;
        UUID environment;
        uint64_t numReferences;
        uint64_t numDataBytes;
        uint64_t flags;

    public:
        ObjectInfo(UUID id, UUID type, uint64_t numDataBytes):
            id{id},
            type{type},
            numReferences{0},
            numDataBytes{numDataBytes},
            flags{0} { }
};

class Object {
    private:
        ObjectInfo info;

    public:
        Object(UUID type) :
            info(random<UUID>(), type, 0) {}

        size_t sizeInMemory();
};


#endif // OBJECT_HPP
