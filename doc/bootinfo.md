# BootInfo

The BootInfo is the structure that is passed by the bootloader to the kernel. The structure is defined [here](src/loader/bootinfo.h) for the loader and [here](src/kernel/setup/bootinfo.hpp) for the kernel. It contains information about:
- the framebuffer (needed if debugging is enabled)
- the memory management frame lists, including [pre-allocated frames](#pre-allocated-frames)
- TODO: initDataFile

# Pre-allocated Frames
There will be 16 frames of persistent memory that can be used before memory management is fully setup. Current usage is as follows:
- `0` `System` object
- `1` initial node of used physical frame stack
- `2-15` reserved

