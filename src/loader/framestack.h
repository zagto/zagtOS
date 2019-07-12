#ifndef FRAMESTACK_H
#define FRAMESTACK_H

#include <efi.h>
#include <util.h>

#define ENTRIES_PER_FRAME_STACK_NODE (PAGE_SIZE / sizeof(UINTN) - 1)

struct FrameStackNode {
    struct FrameStackNode *next;
    UINTN entries[ENTRIES_PER_FRAME_STACK_NODE];
};

struct FrameStack {
    struct FrameStackNode *head;
    UINTN addIndex;
    UINTN lock; // not used in bootloader
};

// random non-canonical address, as NULL = 0 may be a valid physical address of a node
#define FRAMESTACK_NULL ((struct FrameStackNode *)0x1337affe1337affe)


void FrameStackPush(struct FrameStack *self, UINTN address);
UINTN FrameStackPop(struct FrameStack *self);

void FrameStackPrepareForKernel(struct FrameStack *self);

#endif // FRAMESTACK_H
