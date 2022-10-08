#!/bin/bash
set -e

mkdir -p out/$ARCH
lz4 --force build/$ARCH/loader/android/vmlinux out/$ARCH/zImage-lz4
mkbootimg --kernel out/$ARCH/zImage-lz4 --ramdisk initrd.img --dtb sunfish.dtb --cmdline 'nothing' --header_version 2 -o boot-sunfish.img
