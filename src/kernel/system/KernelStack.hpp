#ifndef KERNELSTACK_HPP
#define KERNELSTACK_HPP


class alignas(PAGE_SIZE) KernelStack
{
    uint8_t data[PAGE_SIZE];
};

#endif // KERNELSTACK_HPP
