#include <system/System.hpp>
#include <memory/FrameStack.hpp>
#include <paging/MasterPageTable.hpp>


bool FrameStack::isEmpty() {
    return head->next == nullptr && addIndex == 0;
}


void FrameStack::push(PhysicalAddress address) {
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
    LockHolder lh(lock);

    // other CPUs might have remapped the head page
    MasterPageTable::invalidateLocally(KernelVirtualAddress(head));

    if (isEmpty()) {
        Panic();
    }

    if (addIndex == 0) {
        cout << "Danger: returning old phyiscal head" << endl;
        Node *oldHead = head->next;

        head = head->next;
        /* zero next pointer so the result page is fully zeroed in case of clean frame stack pop */
        head->next = nullptr;
        addIndex = Node::NUM_ENTRIES;

        /* re-use old head node frame as result */
        cout << "pop2\n";
        return PhysicalAddress::fromIdentitdyMappedPointer(oldHead);
    } else {
        addIndex--;
        PhysicalAddress result = head->entries[addIndex];
        // zero so empty nodes can be re-used as result pages even on clean stack
        head->entries[addIndex] = 0;

        return result;
    }
}
