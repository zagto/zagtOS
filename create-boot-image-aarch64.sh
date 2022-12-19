#!/bin/bash
set -e

mkdir -p out/$ARCH
lz4 --force build/$ARCH/loader/android/vmlinux out/$ARCH/zImage-lz4
cat build/$ARCH/kernel/ZAGTKERN.ZBN build/$ARCH/SystemEnvironment/SYSENV.ZBN > out/$ARCH/initrd.img
mkbootimg --kernel out/$ARCH/zImage-lz4 --ramdisk out/$ARCH/initrd.img --dtb sunfish.dtb --cmdline 'nothing' --header_version 2 -o boot-sunfish.img
