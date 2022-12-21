#!/bin/sh
set -e

if [ `uname -s` == "Linux" ]; then
    ACCEL="-enable-kvm"
elif [ `uname -s` == "Darwin" ]; then
    ACCEL="-accel hvf"
elif which winver; then
    ACCEL="-accel hax"
else
    echo "Unknown operating system, can't figure out Qemu acceleration method"
    exit 1
fi

qemu-system-x86_64 \
    -drive if=pflash,format=raw,unit=0,file=/usr/share/ovmf/x64/OVMF_CODE.fd,readonly=on \
    -drive if=pflash,format=raw,unit=1,file=/usr/share/ovmf/x64/OVMF_VARS.fd,readonly=on \
    -m 512 \
    -net none \
    $ACCEL \
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
