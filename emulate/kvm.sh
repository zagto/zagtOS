#!/bin/sh
set -e
qemu-system-x86_64 \
    -drive if=pflash,format=raw,unit=0,file=/usr/share/ovmf/x64/OVMF_CODE.fd,readonly=on \
    -drive if=pflash,format=raw,unit=1,file=/usr/share/ovmf/x64/OVMF_VARS.fd,readonly=on \
    -m 512 \
    -net none \
    -enable-kvm \
    -drive id=disk,file=out/disk.img,if=none \
    -device ahci,id=ahci \
    -device ide-drive,drive=disk,bus=ahci.0 \
    -smp 2 \
    -serial telnet:localhost:30000,server \
    -machine q35 \
    -no-reboot \
     -no-reboot -no-shutdown   -d int,guest_errors,cpu_reset \
    -s &
build/DebugBridge/DebugBridge
