# Memory Layout

## Regions
In general, address space is divided into the following regions of fixed size:
- User Space
  - Task Executable Image + Data Heap, etc...
  - IPC Mailbox
- Kernel Space
  - Kernel Executable Image (.text, .data, ...)
  - (optional) Debug Framebuffer
  - Kernel Persistant Data Structures
    This means things like information about tasks, system totology, memory management, etc... everything that should be preserved when the kernel image is updated.

## Address Ranges
### 64-bit (x86_64)
- `0x0000000000000000 - 0x00007fffffffffff` User Space
- `0xffff800000000000 - 0xffff8fffffffffff` Kernel Image
- `0xffff900000000000 - 0xffff9fffffffffff` Debug Framebuffer
- `0xffffa00000000000 - 0xffffa0003fffffff` Kernel Persistant Data

### 32-bit (planned)
- `0x00000000 - 0xdfffffff` User Space
- `0xe0000000 - 0xe7ffffff` Kernel Image
- `0xe8000000 - 0xefffffff` Debug Framebuffer
- `0xf0000000 - 0xffffffff` Kernel Persistant Data

