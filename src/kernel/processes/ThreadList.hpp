#pragma once

#include <common/panic.hpp>

class Thread;

namespace threadList {
    class AbstractList {};

    class Receptor {
    private:
        template <Receptor Thread::*>
        friend class List;
        template <Receptor Thread::*>
        friend class Iterator;

        AbstractList *list;
        Thread *previous;
        Thread *next;
    };

    template <Receptor Thread::*ReceptorMember>
    class Iterator {
    private:
        Thread *item;

    public:
        Iterator(Thread *thread) {
            item = thread;
        }
        bool operator!=(const Iterator &other) {
            return item != other.item;
        }
        Thread *operator++() {
            assert(item != nullptr);
            item = (item->*ReceptorMember).next;
            assert(item != nullptr);
            return item;
        }
        Thread *operator++(int) {
            assert(item != nullptr);
            Thread *copy = item;
            item = (item->*ReceptorMember).next;
            return copy;
        }
        Thread *operator*() {
            assert(item != nullptr);
            return item;
        }
    };

    template <Receptor Thread::*ReceptorMember>
    class List : AbstractList {
    private:
        Thread *head{nullptr};
        Thread *tail{nullptr};

    public:
        Iterator<ReceptorMember> begin() {
            return {head};
        }

        Iterator<ReceptorMember> end() {
            return {nullptr};
        }

        void append(Thread *thread) {
            Receptor &receptor = thread->*ReceptorMember;
            assert(receptor.list == nullptr);
            assert(receptor.previous == nullptr);
            assert(receptor.next == nullptr);

            if (tail) {
                receptor.next = thread;
                receptor.previous = tail;
                tail = thread;
            } else {
                head = thread;
                tail = thread;
            }
            receptor.list = this;
        }

        void remove(Thread *thread) {
            Receptor &receptor = thread->*ReceptorMember;

            assert(receptor.previous || thread == head);
            assert(receptor.next || thread == tail);
            assert(receptor.list == this);

            if (thread == head) {
                head = receptor.next;
            } else {
                (receptor.previous->*ReceptorMember).next = receptor.next;
            }
            if (thread == tail) {
                tail = receptor.previous;
            } else {
                (receptor.next->*ReceptorMember).previous = receptor.previous;
            }
            receptor.previous = nullptr;
            receptor.next = nullptr;
            receptor.list = nullptr;
        }

        Thread *pop() {
            assert(!empty());

            Receptor &receptor = head->*ReceptorMember;
            assert(receptor.previous == nullptr);
            assert(receptor.list == this);

            Thread *result = head;
            head = receptor.next;
            if (!head) {
                tail = nullptr;
            } else {
                (head->*ReceptorMember).previous = nullptr;
            }
            receptor.next = nullptr;
            receptor.list = nullptr;
            return result;
        }

        bool empty() const {
            assert((head == nullptr) == (tail == nullptr));
            return head == nullptr;
        }
    };
}
