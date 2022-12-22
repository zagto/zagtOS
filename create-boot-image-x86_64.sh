#!/bin/bash
set -e

mkdir -p out/$ARCH/esp/EFI/BOOT
cp build/$ARCH/loader/efi/BOOTX64.EFI out/$ARCH/esp/EFI/BOOT/BOOTX64.EFI
cp build/$ARCH/loader/multiboot/zagtos-multiboot-loader out/$ARCH/esp/MULTBOOT.ELF
cp build/$ARCH/kernel/ZAGTKERN.ZBN out/$ARCH/esp/
cp build/$ARCH/SystemEnvironment/SYSENV.ZBN out/$ARCH/esp/
cp src/loader/config/grub.cfg out/$ARCH/esp/

# create image
dd if=/dev/zero of=out/$ARCH/disk.img bs=1M count=203
dd if=/dev/zero of=out/$ARCH/esp.img bs=1M count=200
out/$ARCH/toolchain/image-generation/bin/mformat -F -i out/$ARCH/esp.img
out/$ARCH/toolchain/image-generation/bin/mcopy -s -i out/$ARCH/esp.img out/$ARCH/esp/* ::/
out/$ARCH/toolchain/image-generation/sbin/parted -s out/$ARCH/disk.img mklabel gpt mkpart ESP fat32 1MiB 201MiB set 1 esp on mkpart bios_grub 201MiB 202MiB set 2 bios_grub on
dd if=out/$ARCH/esp.img of=out/$ARCH/disk.img bs=1M seek=1 count=200 conv=notrunc

# Grub for BIOS
grub-mkimage -O i386-pc --verbose -p /boot/grub -c src/loader/config/grub-early.cfg normal part_gpt multiboot2 biosdisk fat configfile gfxterm > out/$ARCH/grub-core.img
printf '\x00\x48\x06\x00' > out/$ARCH/grub-core-sector
printf '\x01\x48\x06\x00' > out/$ARCH/grub-core-sector2

dd if=/usr/lib/grub/i386-pc/boot.img of=out/$ARCH/disk.img bs=1 count=446 conv=notrunc
dd if=out/$ARCH/grub-core-sector of=out/$ARCH/disk.img bs=1 seek=92 count=4 conv=notrunc

dd if=out/$ARCH/grub-core-sector2 of=out/$ARCH/grub-core.img bs=1 seek=$((512-12)) count=4 conv=notrunc
dd if=out/$ARCH/grub-core.img of=out/$ARCH/disk.img bs=1 seek=201M conv=notrunc
