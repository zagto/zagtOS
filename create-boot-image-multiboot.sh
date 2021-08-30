#!/bin/bash
set -e

mkdir -p out/multiboot
cp build/loader/multiboot/zagtos-multiboot-loader out/multiboot/
cp build/kernel/ZAGTKERN.ZBN out/multiboot/
cp build/SystemEnvironment/SYSENV.ZBN out/multiboot/
# create image
dd if=/dev/zero of=out/multiboot-disk.img bs=1M count=202
dd if=/dev/zero of=out/multiboot.img bs=1M count=200
mkfs.vfat -F32 out/multiboot.img
parted -s out/multiboot-disk.img mklabel msdos mkpart primary fat32 1MiB 201MiB set 1 boot on
grub-install --root-directory=$PWD/out/multiboot --no-floppy --target=i386-pc --modules="normal part_msdos multiboot biosdev" $PWD/out/multiboot-disk.img
mcopy -s -i out/multiboot.img out/multiboot/* ::/
dd if=out/multiboot.img of=out/multiboot-disk.img bs=1M seek=1 count=200 conv=notrunc

