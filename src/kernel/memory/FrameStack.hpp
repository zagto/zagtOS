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

        FrameStack() :
            head{nullptr},
            addIndex{0} {} /* (invalid state) */
        FrameStack(const hos_v1::FrameStack &handOver) :
            head{reinterpret_cast<Node *>(handOver.head)},
            addIndex{handOver.addIndex} {}
        FrameStack(Node *head, size_t addIndex) :
            head{head},
            addIndex{addIndex} {}

        FrameStack(FrameStack &other) = delete;
        FrameStack(FrameStack &&other) {
            head = other.head;
            addIndex = other.addIndex;
            other.head = nullptr;
            other.addIndex = 0;
        }
        void operator=(FrameStack &other) = delete;
        void operator=(FrameStack &&other) {
            assert(head == nullptr && addIndex == 0); /* (invalid state) */
            head = other.head;
            addIndex = other.addIndex;
            other.head = nullptr;
            other.addIndex = 0;
        }
        ~FrameStack() {
            assert(head == nullptr && addIndex == 0); /* (invalid state) */
        }

        bool isEmpty();
        PhysicalAddress pop();
        void push(PhysicalAddress address);
    };
}

using frameStack::FrameStack;
