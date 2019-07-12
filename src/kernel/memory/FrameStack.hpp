#ifndef FRAMESTACK_HPP
#define FRAMESTACK_HPP

#include <lib/Lock.hpp>

namespace frameStack {
    class Node {
    public:
        static const usize NUM_ENTRIES{PAGE_SIZE / sizeof(PhysicalAddress) - 1};

        Node *next;
        PhysicalAddress entries[NUM_ENTRIES];

        Node(Node *nextNode) : next{nextNode} {}
    };

    class FrameStack {
    private:
        Node *head;
        usize addIndex;
        Lock lock;

    public:
        ~FrameStack() {
            // Can't destruct the frame stack
            Panic();
        }

        bool isEmpty();
        PhysicalAddress pop();
        void push(PhysicalAddress address);
        void print(){
            Log << "this: " << this << "\n";
            Log << "head: " << head << "\n";
            Log << "addIndex: " << addIndex << "\n";

        }
    };
}

using frameStack::FrameStack;

#endif // FRAMESTACK_HPP
