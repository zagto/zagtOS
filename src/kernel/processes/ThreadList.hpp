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

        AbstractList *list{nullptr};
        Thread *previous{nullptr};
        Thread *next{nullptr};
    };

    template <Receptor Thread::*ReceptorMember>
    class Iterator {
    private:
        Thread *item;

    public:
        Iterator(Thread *thread = nullptr) noexcept {
            item = thread;
        }
        bool operator!=(const Iterator &other) noexcept {
            return item != other.item;
        }
        bool operator==(const Iterator &other) noexcept {
            return item == other.item;
        }
        Iterator &operator++() noexcept {
            assert(item != nullptr);
            item = (item->*ReceptorMember).next;
            return *this;
        }
        Iterator operator++(int) noexcept {
            assert(item != nullptr);
            Iterator copy = item;
            item = (item->*ReceptorMember).next;
            return copy;
        }
        Thread *operator*() noexcept {
            assert(item != nullptr);
            return item;
        }
        Thread *operator->() noexcept {
            assert(item != nullptr);
            return item;
        }
        Thread *get() const noexcept {
            return item;
        }
    };

    template <Receptor Thread::*ReceptorMember>
    class List : AbstractList {
    private:
        Thread *head{nullptr};
        Thread *tail{nullptr};

    public:
        Iterator<ReceptorMember> begin() noexcept {
            return {head};
        }

        Iterator<ReceptorMember> end() noexcept {
            return {nullptr};
        }

        void append(Thread *thread) noexcept {
            Receptor &receptor = thread->*ReceptorMember;
            assert(receptor.list == nullptr);
            assert(receptor.previous == nullptr);
            assert(receptor.next == nullptr);

            if (tail) {
                (tail->*ReceptorMember).next = thread;
                receptor.previous = tail;
                tail = thread;
            } else {
                head = thread;
                tail = thread;
            }
            receptor.list = this;
        }

        void insertBefore(Thread *existingThread, Thread *toInsert) noexcept {
            Receptor &toInsertReceptor = toInsert->*ReceptorMember;
            assert(toInsertReceptor.list == nullptr);
            assert(toInsertReceptor.previous == nullptr);
            assert(toInsertReceptor.next == nullptr);

            Receptor &existingReceptor = existingThread->*ReceptorMember;
            assert(existingReceptor.list == this);

            toInsertReceptor.next = existingThread;
            toInsertReceptor.previous = existingReceptor.previous;
            toInsertReceptor.list = this;
            if (existingReceptor.previous == nullptr) {
                /* existing Thread was first Thread */
                head = toInsert;
            } else {
                (toInsertReceptor.previous->*ReceptorMember).next = toInsert;
            }
            existingReceptor.previous = toInsert;
        }

        void remove(Thread *thread) noexcept {
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

        Thread *pop() noexcept {
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

        bool empty() const noexcept {
            assert((head == nullptr) == (tail == nullptr));
            return head == nullptr;
        }
    };
}
