#pragma once
#include <common/Region.hpp>

static constexpr size_t KERNEL_HEAP_SIZE = 1024 * 1024 * 1024;

static constexpr Region KernelImageRegion(
    0xffffff8000000000,
    0x0000001000000000
);
static constexpr Region FramebufferRegion(
    0xffffff9000000000,
    0x0000000800000000
);
static constexpr Region SerialRegion(
    0xffffff9800000000,
    0x0000000800000000
);
static constexpr Region KernelHeapRegion(
    0xffffffa000000000,
    KERNEL_HEAP_SIZE
);
static constexpr Region UserSpaceRegion(
    0x0000000000000000,
    0x0000008000000000
);

static constexpr Region IdentityMapping(
    0xffffffb000000000,
    0x0000004ffffff000
);

static const Region KernelRegions[]{
    KernelImageRegion,
    FramebufferRegion,
    KernelHeapRegion,
};

static const Region AllRegions[]{
    KernelImageRegion,
    FramebufferRegion,
    SerialRegion,
    KernelHeapRegion,
    UserSpaceRegion,
    IdentityMapping
};

static constexpr Region LoaderRegion = UserSpaceRegion;
