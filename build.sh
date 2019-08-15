#!/bin/bash
set -e

source config.sh

# remove old disk
rm -rf out/*.img
rm -rf out/esp

# enable toolchains that may be built
export PATH="$SHBUILD_ROOT/out/toolchain/loader-${ARCH}/bin:$SHBUILD_ROOT/out/toolchain/kernel-${ARCH}/bin:$SHBUILD_ROOT/out/toolchain/system-${ARCH}/bin:$PATH"
export SYSROOT="$SHBUILD_ROOT/out/toolchain/sysroot-${ARCH}"

export KERNEL_CFLAGS_x86_64="-mcmodel=large -mno-red-zone -ffixed-r15"

# install headers first
rm -rf "$SYSROOT"
mkdir -p "$SYSROOT/include"

for mod in $MODULES
do
    pushd "src/$mod"
    export SHBUILD_BUILD_DIR="$SHBUILD_ROOT/build/$mod"
    mkdir -p "$SHBUILD_BUILD_DIR"
    if [ -f build-script ]; then
        ./build-script
    elif [ -f Makefile ]; then
        make -j "$PARALLEL_JOBS"
        make install
    fi
    popd
done

mkdir -p out/esp/EFI/BOOT
cp build/loader/BOOTX64.EFI out/esp/EFI/BOOT/
cp build/kernel/SHKERNEL.BIN out/esp/
cp build/SystemEnvironment/SystemEnvironment out/esp/INIT.BIN
# create image
dd if=/dev/zero of=out/disk.img bs=1M count=34
dd if=/dev/zero of=out/esp.img bs=1M count=33
mkfs.vfat -F32 out/esp.img
mcopy -s -i out/esp.img out/esp/* ::/
parted -s out/disk.img mklabel gpt mkpart ESP fat32 1MiB 33MiB set 1 esp on
dd if=out/esp.img of=out/disk.img bs=1M seek=1 count=33

