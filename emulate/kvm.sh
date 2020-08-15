#!/bin/sh
set -e
qemu-system-x86_64 \
    -drive if=pflash,format=raw,unit=0,file=/usr/share/ovmf/x64/OVMF_CODE.fd,readonly=on \
    -drive if=pflash,format=raw,unit=1,file=/usr/share/ovmf/x64/OVMF_VARS.fd,readonly=on \
    -net none \
    -enable-kvm \
    -drive id=disk,file=out/disk.img,if=none \
    -device ahci,id=ahci \
    -device ide-drive,drive=disk,bus=ahci.0 \
    -serial stdio \
    -smp 2 \
    -machine q35 \
    -no-reboot \
     -no-reboot -no-shutdown   -d guest_errors,cpu_reset \
    -s
