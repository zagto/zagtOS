#ifndef LIST_HPP
#define LIST_HPP

#include <common/panic.hpp>
#include <common/inttypes.hpp>
#include <log/logger.hpp>

namespace list {
    template<class T> class List;

    template<typename ElementType> class Node {
    public:
        Node *nextNode{nullptr};
        Node *previousNode{nullptr};
        ElementType *element;

        Node(ElementType *element) : element{element} {};
    };


    template<typename ElementType> class List {
    private:
        Node<ElementType> *firstNode{nullptr};
        Node<ElementType> *lastNode{nullptr};

    public:
        void pushBack(ElementType *element) {
            Node<ElementType> *node = new Node<ElementType>(element);
            node->previousNode = lastNode;
            lastNode = node;
            if (firstNode) {
                node->previousNode->nextNode = node;
            } else {
                firstNode = node;
            }
        }

        ElementType *popFront() {
            assert(!isEmpty());

            Node<ElementType> *resultNode = firstNode;
            firstNode = resultNode->nextNode;
            if (firstNode == nullptr) {
                assert(lastNode == resultNode);
                lastNode = nullptr;
            } else {
                firstNode->previousNode = nullptr;
            }

            ElementType *result = resultNode->element;
            delete resultNode;
            return result;
        }

        void remove(ElementType *elem) {
            Node<ElementType> *node = firstNode;
            while (node && node->element != elem) {
                node = node->nextNode;
            }
            if (!node) {
                log::cout << "Tried to remove Element that is not in list" << log::endl;
                Panic();
            }

            if (node->previousNode) {
                node->previousNode->nextNode = node->nextNode;
            } else {
                firstNode = node->nextNode;
            }
            if (node->nextNode) {
                node->nextNode->previousNode = node->previousNode;
            } else {
                lastNode = node->previousNode;
            }
        }

        ElementType *operator[](size_t index) {
            Node<ElementType> *node = firstNode;
            while(index) {
                node = node->next();
                index--;
            }
            return node->element;
        }

        bool isEmpty() {
            return firstNode == nullptr;
        }
    };
}

using list::List;

#endif // LIST_HPP
