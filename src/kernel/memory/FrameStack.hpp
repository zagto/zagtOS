#pragma once

#include <common/inttypes.hpp>
#include <common/addresses.hpp>
#include <setup/HandOverState.hpp>

namespace frameStack {
    struct Node {
        static const size_t NUM_ENTRIES{PAGE_SIZE / sizeof(PhysicalAddress) - 1};

        Node *next;
        PhysicalAddress entries[NUM_ENTRIES];

        Node(Node *nextNode) : next{nextNode} {}
    };

    struct FrameStack {
        Node *head;
        size_t addIndex;

        FrameStack(const hos_v1::FrameStack &handOver) :
            head{reinterpret_cast<Node *>(handOver.head)},
            addIndex{handOver.addIndex} {}
        FrameStack(Node *head, size_t addIndex) :
            head{head},
            addIndex{addIndex} {}

        ~FrameStack() {
            // Can't destruct the frame stack
            Panic();
        }

        bool isEmpty();
        PhysicalAddress pop();
        void push(PhysicalAddress address);
    };
}

using frameStack::FrameStack;
