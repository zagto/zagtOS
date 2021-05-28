#pragma once

#include <common/common.hpp>
#include <system/System.hpp>

static_assert(MAX_NUM_PROCESSORS % PLATFORM_BITS == 0);

class CPUMask {
private:
    static constexpr size_t numBlocks = MAX_NUM_PROCESSORS / PLATFORM_BITS;

    class Iterator {
    private:
        CPUMask &container;
        size_t position;

        bool isInRange() const;

    public:
        Iterator(CPUMask &container, size_t position);

        bool operator!=(const Iterator &other) const;
        void operator++();
        Processor &operator*();
    };

    size_t data[numBlocks]{0};

    size_t indexFor(size_t processorID) const;
    size_t bitFor(size_t processorID) const;

public:
    CPUMask();
    CPUMask(const Processor &processor);

    void operator|=(const CPUMask &other);
    bool operator[](size_t processorID);
    Iterator begin();
    Iterator end();
};
