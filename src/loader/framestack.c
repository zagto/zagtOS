#include <efi.h>
#include <physical-memory.h>
#include <virtual-memory.h>
#include <framestack.h>


static void addNewStackNode(struct FrameStack *self) {
    struct FrameStackNode *newNode = (struct FrameStackNode *)AllocatePhysicalFrame();
    newNode->next = self->head;
    self->head = newNode;
    self->addIndex = 0;
}


void FrameStackPush(struct FrameStack *self, UINTN address) {
    if (self->addIndex == ENTRIES_PER_FRAME_STACK_NODE) {
        addNewStackNode(self);

        /* as this is the physical frame stack, we should use the slot left over by allocating
         * the frame for the new node */
        self->head->next->entries[ENTRIES_PER_FRAME_STACK_NODE - 1] = address;
        return;
    }
    self->head->entries[self->addIndex] = address;
    self->addIndex++;
}


UINTN FrameStackPop(struct FrameStack *self) {
    if (self->head->next == FRAMESTACK_NULL && self->addIndex == 0) {
        Log("No frames to allocate");
        Halt();
    }

    if (self->addIndex == 0) {
        struct FrameStackNode *oldHead = self->head;
        self->head = self->head->next;
        self->addIndex = ENTRIES_PER_FRAME_STACK_NODE;

        /* as we are the stack used to keep track of physical frames, de-allocating the old head
         * frame would lead to an endless recursion of allocating/de-allocating it.
         * Use it as the result directly instead */
        oldHead->next = 0;
        return (UINTN)oldHead;
    }

    self->addIndex--;
    UINTN result = self->head->entries[self->addIndex];
    self->head->entries[self->addIndex] = 0;
    return result;
}


/* Add IDENTITY_MAPPING_BASE to all pointers so they are useful in the kernel */
void FrameStackPrepareForKernel(struct FrameStack *self) {
    struct FrameStackNode *node = self->head;
    if (node == FRAMESTACK_NULL) {
        self->head = 0;
        return;
    }
    self->head = (struct FrameStackNode *)((UINTN)self->head + IDENTITY_MAPPING_BASE);

    while (node->next != FRAMESTACK_NULL) {
        struct FrameStackNode *next = node->next;
        node->next = (struct FrameStackNode *)((UINTN)node->next + IDENTITY_MAPPING_BASE);

        node = next;
    }
    node->next = 0;
}
