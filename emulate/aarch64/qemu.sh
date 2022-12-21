#!/bin/sh
set -e
qemu-system-aarch64 \
    -machine virt \
    -m 512 \
    -cpu cortex-a53 \
    -kernel out/aarch64/vmlinux -initrd out/aarch64/initrd.img \
    -smp 2 \
    -serial telnet:localhost:30000,server \
    -no-reboot -no-shutdown -d int,guest_errors,cpu_reset \
    -s &
build/aarch64/DebugBridge/DebugBridge
