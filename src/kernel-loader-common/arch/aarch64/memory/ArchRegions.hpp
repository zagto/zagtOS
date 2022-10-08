#pragma once
#include <common/Region.hpp>

static constexpr size_t KERNEL_HEAP_SIZE = 1024 * 1024 * 1024;

static constexpr Region KernelImageRegion(
    0xffff800000000000,
    0x0000100000000000
);
static constexpr Region FramebufferRegion(
    0xffff900000000000,
    0x0000100000000000
);
static constexpr Region KernelHeapRegion(
    0xffffa00000000000,
    KERNEL_HEAP_SIZE
);
static constexpr Region UserSpaceRegion(
    0x0000000000000000,
    0x0000800000000000
);

static constexpr Region IdentityMapping(
    0xffffb00000000000,
    0x00004ffffffff000
);

static const Region KernelRegions[]{
    KernelImageRegion,
    FramebufferRegion,
    KernelHeapRegion,
};

static const Region AllRegions[]{
    KernelImageRegion,
    FramebufferRegion,
    KernelHeapRegion,
    UserSpaceRegion,
    IdentityMapping
};

static constexpr Region LoaderRegion = UserSpaceRegion;
