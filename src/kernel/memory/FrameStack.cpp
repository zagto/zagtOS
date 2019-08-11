#include <system/System.hpp>
#include <memory/FrameStack.hpp>
#include <paging/PagingContext.hpp>


bool FrameStack::isEmpty() {
    assert(CurrentSystem.memory.frameManagementLock.isLocked());
    return head->next == nullptr && addIndex == 0;
}


void FrameStack::push(PhysicalAddress address) {
    assert(CurrentSystem.memory.frameManagementLock.isLocked());
    LockHolder lh(lock);

    if (addIndex == Node::NUM_ENTRIES) {
        Node *oldHead = head;

        // re-use frame for new node instead of freeing it
        head = new (address.identityMapped()) Node(oldHead);
        addIndex = 0;
    } else {
        head->entries[addIndex] = address;
        addIndex++;
    }
}


PhysicalAddress FrameStack::pop() {
    assert(CurrentSystem.memory.frameManagementLock.isLocked());
    LockHolder lh(lock);

    // other CPUs might have remapped the head page
    PagingContext::invalidateLocally(KernelVirtualAddress(head));

    if (isEmpty()) {
        Panic();
    }

    if (addIndex == 0) {
        Node *oldHead = head;
        head = head->next;

        /* zero next pointer so the result page is fully zeroed in case of clean frame stack pop */
        oldHead->next = nullptr;
        addIndex = Node::NUM_ENTRIES;

        /* re-use old head node frame as result */
        return PhysicalAddress::fromIdentitdyMappedPointer(oldHead);
    } else {
        addIndex--;
        PhysicalAddress result = head->entries[addIndex];
        // zero so empty nodes can be re-used as result pages even on clean stack
        head->entries[addIndex] = 0;

        return result;
    }
}
