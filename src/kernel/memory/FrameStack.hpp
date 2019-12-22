#pragma once

#include <common/common.hpp>

namespace frameStack {
    class Node {
    public:
        static const size_t NUM_ENTRIES{PAGE_SIZE / sizeof(PhysicalAddress) - 1};

        Node *next;
        PhysicalAddress entries[NUM_ENTRIES];

        Node(Node *nextNode) : next{nextNode} {}
    };

    class FrameStack {
    private:
        Node *head;
        size_t addIndex;

    public:
        ~FrameStack() {
            // Can't destruct the frame stack
            Panic();
        }

        bool isEmpty();
        PhysicalAddress pop();
        void push(PhysicalAddress address);
        void print(){
            cout << "this: " << this << "\n";
            cout << "head: " << head << "\n";
            cout << "addIndex: " << addIndex << "\n";

        }
    };
}

using frameStack::FrameStack;
