#ifndef OBJECT_HPP
#define OBJECT_HPP

#include <common/common.hpp>

class Object;

class UUID {
    private:
        u64 data[2];

    public:
        UUID() :
            data{0, 0} {}

        UUID(u64 part1, u64 part2) :
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
        u64 numReferences;
        u64 numDataBytes;
        u64 flags;

    public:
        ObjectInfo(UUID id, UUID type, u64 numDataBytes):
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

        usize sizeInMemory();
};


#endif // OBJECT_HPP
