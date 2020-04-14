#!/bin/bash
set -e

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

