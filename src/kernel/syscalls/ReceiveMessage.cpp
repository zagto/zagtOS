#include <syscalls/ReceiveMessage.hpp>
#include <system/System.hpp>
#include <system/Processor.hpp>


void mergeSort(vector<size_t> &container, vector<shared_ptr<Port>> &ports) {
    vector<size_t> scratch(container.size());

    size_t partSize = 2;
    while (partSize / 2 < container.size()) {
        for (size_t partIndex = 0; partIndex * partSize < container.size(); partIndex++) {
            size_t leftPos = partIndex * partSize;
            size_t rightPos = partIndex * partSize + partSize / 2;
            size_t outPos = leftPos;

            /* merge */
            size_t leftEnd = rightPos;
            size_t rightEnd = min(leftPos + partSize, container.size());

            while (leftPos < leftEnd && rightPos < rightEnd) {
                if (ports[container[leftPos]].get() <= ports[container[rightPos]].get()) {
                    scratch[outPos] = move(container[leftPos]);
                    outPos++;
                    leftPos++;
                } else {
                    scratch[outPos] = move(container[rightPos]);
                    outPos++;
                    rightPos++;
                }
            }

            while (leftPos < leftEnd) {
                scratch[outPos] = move(container[leftPos]);
                outPos++;
                leftPos++;
            }

            while (rightPos < rightEnd) {
                scratch[outPos] = move(container[rightPos]);
                outPos++;
                rightPos++;
            }
        }

        swap(scratch, container);
        partSize *= 2;
    }
}

size_t ReceiveMessage(const shared_ptr<Process> &process,
                   size_t _portHandles,
                   size_t count,
                   size_t,
                   size_t,
                   size_t) {
    vector<uint32_t> portHandles(count);

    process->addressSpace.copyFrom(reinterpret_cast<uint8_t *>(portHandles.data()),
                                   _portHandles,
                                   count * sizeof(uint32_t));
    vector<shared_ptr<Port>> unsortedPorts(count);
    vector<shared_ptr<Port>> ports(count);
    vector<size_t> originalIndexes(count);

    for (size_t index = 0; index < count; index++) {
        unsortedPorts[index] = process->handleManager.lookup<shared_ptr<Port>>(portHandles[index]);
        originalIndexes[index] = index;
    }

    mergeSort(originalIndexes, unsortedPorts);

    for (size_t index = 0; index < count; index++) {
        cout << originalIndexes[index] << " -> " << index << endl;
        ports[index] = move(unsortedPorts[originalIndexes[index]]);
    }
    unsortedPorts.resize(0);

    /* Danger: non-RAII locking. This is assuming the ports are sorted to not mess up lock order */
    for (auto &port: ports) {
        port->lock.lock();
    }

    size_t resultIndex = 0;
    unique_ptr<Message> messageResult;
    try {
        for (size_t index = 0; index < ports.size(); index++) {
            messageResult = ports[index]->getMessage();
            /* break if we found a message */
            if (messageResult) {
                resultIndex = index;
                break;
            }
        }

        /* no pending message, Thread has to wait */
        if (!messageResult) {
            /* Danger Zone: Asymmetric lock/unlock: These two locks will not be unlocked once this
             * method leaves, but once the thread state is discarded. */
            scoped_lock sl(KernelInterruptsLock);
            Processor *processor = CurrentProcessor();
            scoped_lock sl2(processor->scheduler.lock);

            Thread *thread = CurrentThread();
            processor->scheduler.removeActiveThread();
            thread->setState(Thread::State::WaitMessage());

            for (size_t index = 0; index < ports.size(); index++) {
                ports[index]->setWaitingThread(thread, originalIndexes[index]);
            }

            thread->waitingPorts = move(ports);

            /* unlock the non-RAII port locks */
            for (auto &port: thread->waitingPorts) {
                port->lock.unlock();
            }
            throw DiscardStateAndSchedule(thread, move(sl), move(sl2));
        }
    } catch (...) {
        /* unlock the non-RAII port locks */
        for (auto &port: ports) {
            port->lock.unlock();
        }
        throw;
    }

    /* success - we got a message */
    /* unlock the non-RAII port locks */
    for (auto &port: ports) {
        port->lock.unlock();
    }

    cout << "returning resultIndex " << resultIndex << " of " << count << endl;
    /* Transfer the resultIndex variable into the message info */
    process->addressSpace.copyTo(messageResult->infoAddress.value(),
                                 reinterpret_cast<uint8_t *>(&resultIndex),
                                 sizeof(size_t),
                                 true);
    return messageResult->infoAddress.value();
}
