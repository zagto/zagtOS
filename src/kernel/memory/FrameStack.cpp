#include <memory/FrameStack.hpp>
#include <log/Logger.hpp>


bool FrameStack::isEmpty() {
#ifndef ZAGTOS_LOADER
    return head == nullptr;
#else
    return reinterpret_cast<size_t>(head) == PhysicalAddress::NULL;
#endif
}


void FrameStack::push(PhysicalAddress address) {
    if (addIndex == Node::NUM_ENTRIES) {
        Node *oldHead = head;

        /* re-use frame for new node instead of freeing it */
#ifdef ZAGTOS_LOADER
        head = reinterpret_cast<Node *>(address.value());
#else
        head = reinterpret_cast<Node *>(address.identityMapped().value());
#endif
        head->next = oldHead;
        addIndex = 0;
    } else {
        head->entries[addIndex] = address;
        addIndex++;
    }
}


PhysicalAddress FrameStack::pop() {
    if (isEmpty()) {
        cout << "out of memory" << endl;
        Panic();
    }

    if (addIndex == 0) {
        Node *oldHead = head;
        head = head->next;

        /* zero next pointer so the result page is fully zeroed in case of clean frame stack pop */
        oldHead->next = nullptr;
        addIndex = Node::NUM_ENTRIES;

        /* re-use old head node frame as result */
#ifdef ZAGTOS_LOADER
        return {reinterpret_cast<size_t>(oldHead)};
#else
        //memset(oldHead, 0, PAGE_SIZE);
        return PhysicalAddress::fromIdentitdyMappedPointer(oldHead);
#endif
    } else {
        addIndex--;
        PhysicalAddress result = head->entries[addIndex];
        // zero so empty nodes can be re-used as result pages even on clean stack
        head->entries[addIndex] = 0;

        return result;
    }
}
