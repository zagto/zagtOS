#!/bin/sh
set -e
qemu-system-x86_64 \
    -drive if=pflash,format=raw,unit=0,file=/usr/share/ovmf/x64/OVMF_CODE.fd,readonly=on \
    -drive if=pflash,format=raw,unit=1,file=/usr/share/ovmf/x64/OVMF_VARS.fd,readonly=on \
    -m 512 \
    -net none \
    -enable-kvm \
    -drive id=disk,file=out/x86_64/disk.img,if=none \
    -device ahci,id=ahci \
    -device ide-hd,drive=disk,bus=ahci.0 \
    -smp 2 \
    -serial telnet:localhost:30000,server \
    -machine q35 \
    -cpu host,invtsc,fsgsbase \
    -no-reboot \
     -no-reboot -no-shutdown   -d int,guest_errors,cpu_reset \
    -s &
build/x86_64/DebugBridge/DebugBridge
