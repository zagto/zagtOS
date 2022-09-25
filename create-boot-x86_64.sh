#!/bin/bash
set -e

mkdir -p out/esp/EFI/BOOT
cp build/loader/efi/BOOTX64.EFI out/esp/EFI/BOOT/BOOTX64.EFI
cp build/loader/multiboot/zagtos-multiboot-loader out/esp/MULTBOOT.ELF
cp build/kernel/ZAGTKERN.ZBN out/esp/
cp build/SystemEnvironment/SYSENV.ZBN out/esp/
cp src/loader/config/grub.cfg out/esp/

# create image
dd if=/dev/zero of=out/disk.img bs=1M count=203
dd if=/dev/zero of=out/esp.img bs=1M count=200
mkfs.vfat -F32 out/esp.img
mcopy -s -i out/esp.img out/esp/* ::/
parted -s out/disk.img mklabel gpt mkpart ESP fat32 1MiB 201MiB set 1 esp on mkpart bios_grub 201MiB 202MiB set 2 bios_grub on
dd if=out/esp.img of=out/disk.img bs=1M seek=1 count=200 conv=notrunc

# Grub for BIOS
grub-mkimage -O i386-pc --verbose -p /boot/grub -c src/loader/config/grub-early.cfg normal part_gpt multiboot2 biosdisk fat configfile gfxterm > out/grub-core.img
printf '\x00\x48\x06\x00' > out/grub-core-sector
printf '\x01\x48\x06\x00' > out/grub-core-sector2

dd if=/usr/lib/grub/i386-pc/boot.img of=out/disk.img bs=1 count=446 conv=notrunc
dd if=out/grub-core-sector of=out/disk.img bs=1 seek=92 count=4 conv=notrunc

dd if=out/grub-core-sector2 of=out/grub-core.img bs=1 seek=$((512-12)) count=4 conv=notrunc
dd if=out/grub-core.img of=out/disk.img bs=1 seek=201M conv=notrunc
