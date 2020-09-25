KERNEL_CFLAGS_VARIABLE_NAME="KERNEL_CFLAGS_${ARCH}"
check_toolchain "kernel-${ARCH}" "${ARCH}-elf" 'yes' "-g -O2 ${!KERNEL_CFLAGS_VARIABLE_NAME}"
check_toolchain "system-${ARCH}" "${ARCH}-zagto-zagtos" '' ''
