
## zagtOS - a microkernel operating system written in C++

Current features include:

- SMP
- RTTI, Exceptions, STL-like scoped locks, smart pointers, vectors, and minimal undefined behavoir sanitizer in the kernel
- full runtime support in user space using the [musl](https://www.musl-libc.org/) C library and GNU libstdc++
- GCC cross compiler for the `*-zagtos` target for user space code
- [ZBON](src/zagtos++/zagtos/ZBON.hpp), a binary data serialization format, used for message passing and for program binaries, which are [converted from ELF](src/ConvertELF)
- Memory Management and Futexes with a Linux-like syscall interface
- Message passing IPC which is also used for passing handles for access restricted things, like a region of physical address space, or an IPC Port of another user-space Process
- Interrupt handling by user space Processes (still missing a lot of cases)
- Bootloader/Kernel [handover protocol](src/kernel/setup/HandOverState.hpp) designed with Live Kernel upgrades in mind (currently the bootloader loads a first Process, which contains the following processes, instead of an initrd-like mechanism)
- Bootloader for both UEFI and Multiboot2
- An own [build tool](buildtool.cpp) to track dependencies between subprojects
- DebugBridge that can collect core dumps of Processes over the (virtual) serial port
- Currently only x86_64, but written with portability in mind
- Current area of work: first device drivers (PS/2 and AHCI)


## Building

Requirements:
- everything required to build gcc
- GNU parted, mtools, GRUB to create the disk image

### Installing requirements on Windows (MSYS2)

pacman -S make gcc texinfo diffutils bison flex mpc-devel lz4

### Build

If all of these are installed, you should be able to build with just:

```
make
```

(This has not been tested on many systems yet, please let me know in which way it goes wrong)

Once the build is complete, a BIOS and UEFI bootable disk image is created at `out/disk.img`. Scripts for running the system in different emulaters are in the `emulate` directory, intended to be used from the project root like:

```
./emulate/kvm.sh
```

### VritualBox
For VirtualBox, additional setup of the VM is required. The Paravirtualization type needs to be set to `minimal`, and for the DebugBridge to work, the serial port COM1 needs to be bound to TCP port 30000. The script currently assumes the VM is called `Test`. On launch it coverts the `disk.img` to `disk.vdi` which can be added to the VM.

## Third Party Software

- `src/ACPIHAL/acpica` The [ACPICA](https://www.acpica.org/) ACPI library. Used for user-space ACPI support (GPLv2)
- `src/kernel/memory/dlmalloc.c` dlmalloc Allocator used in the Kernel (Public Domain)
- `src/binutils` GNU Binutils
- `src/gcc` GNU GCC, incldung libgcc, libsupc++, libstdc++
- `src/gmp`, `src/mpc`, `src/mpfr` GCC dependencies, not yet used
- `src/musl` [musl](https://www.musl-libc.org/) C library (MIT/BSD)
- `src/gnu-efi` [GNU EFI](https://sourceforge.net/projects/gnu-efi/), used by zagtOS loader (BSD)

## Licensing

ZagtOS is released under the GNU General Public License (GPLv3)
