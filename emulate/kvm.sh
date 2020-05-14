#!/bin/sh
set -e
qemu-system-x86_64 \
    -drive if=pflash,format=raw,unit=0,file=/usr/share/ovmf/x64/OVMF_CODE.fd,readonly=on \
    -drive if=pflash,format=raw,unit=1,file=/usr/share/ovmf/x64/OVMF_VARS.fd,readonly=on \
    -net none \
    -enable-kvm \
    -hda out/disk.img \
    -serial stdio \
    -smp 2 \
    -machine q35 \
    -no-reboot \
     -no-reboot -no-shutdown   -d guest_errors,cpu_reset \
    -s
