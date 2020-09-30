#include <common/Region.hpp>

static constexpr size_t USER_STACK_SIZE = 2 * 1024 * 1024;
static constexpr size_t USER_STACK_BORDER = 0x1000 * 10;
static constexpr size_t RED_ZONE_SIZE = 128;

static constexpr size_t KERNEL_STATIC_DATA_SIZE = PAGE_SIZE;

static constexpr Region KernelImageRegion(
    0xffff800000000000,
    0x0000100000000000
);
static constexpr Region FramebufferRegion(
    0xffff900000000000,
    0x0000100000000000
);
static const Region KernelStaticDataRegion(
    0xffffa00000000000,
    KERNEL_STATIC_DATA_SIZE
);
static const Region KernelHeapRegion(
    KernelStaticDataRegion.start + KernelStaticDataRegion.length,
    0x0000080000000000
);
static constexpr Region UserSpaceRegion(
    0x0000000000000000,
    0x0000800000000000
);
static constexpr Region UserStackRegion(
    UserSpaceRegion.length - USER_STACK_SIZE,
    USER_STACK_SIZE
);
static constexpr Region UserStackBorderRegion(
    UserSpaceRegion.length - USER_STACK_SIZE - USER_STACK_BORDER,
    USER_STACK_BORDER
);

static constexpr Region IdentityMapping(
    0xffffb00000000000,
    0x00004ffffffff000
);

static const Region KernelRegions[]{
    KernelImageRegion,
    FramebufferRegion,
    KernelStaticDataRegion,
    KernelHeapRegion,
};

static const Region AllRegions[]{
    KernelImageRegion,
    FramebufferRegion,
    KernelStaticDataRegion,
    KernelHeapRegion,
    UserSpaceRegion,
    IdentityMapping
};

static constexpr Region LoaderRegion = UserSpaceRegion;
