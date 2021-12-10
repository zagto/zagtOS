#include <syscalls/ReceiveMessage.hpp>
#include <system/System.hpp>
#include <system/Processor.hpp>


Status mergeSort(vector<shared_ptr<Port>> &container) {
    Status status = Status::OK();
    vector<shared_ptr<Port>> scratch(container.size(), status);
    if (!status) {
        return status;
    }

    size_t partSize = 2;
    while (partSize / 2 < container.size()) {
        for (size_t partIndex = 0; partIndex <= container.size() / partSize; partIndex++) {
            size_t leftPos = partIndex * partSize;
            size_t rightPos = partIndex * partSize + partSize / 2;
            size_t outPos = leftPos;

            /* merge */
            if (rightPos < container.size()) {
                size_t leftEnd = rightPos;
                size_t rightEnd = min(leftPos + partSize, container.size());

                while (leftPos < leftEnd && rightPos < rightEnd) {
                    if (container[leftPos] <= container[rightPos]) {
                        scratch[outPos] = container[leftPos];
                        outPos++;
                        leftPos++;
                    } else {
                        scratch[outPos] = container[rightPos];
                        outPos++;
                        rightPos++;
                    }
                }

                while (leftPos < leftEnd) {
                    scratch[outPos] = container[leftPos];
                    outPos++;
                    leftPos++;
                }

                while (rightPos < rightEnd) {
                    scratch[outPos] = container[rightPos];
                    outPos++;
                    rightPos++;
                }
            }
        }

        swap(scratch, container);
        partSize *= 2;
    }
    return Status::OK();
}

Result<size_t> ReceiveMessage(const shared_ptr<Process> &process,
                   size_t _portHandles,
                   size_t count,
                   size_t,
                   size_t,
                   size_t) {
    Status status = Status::OK();
    vector<uint32_t> portHandles(count, status);
    if (!status) {
        cout << "receiveMessage: Unable to create vector of " << count << " handles: " << status
             << endl;
        return status;
    }

    status = process->addressSpace.copyFrom(reinterpret_cast<uint8_t *>(portHandles.data()),
                                            _portHandles,
                                            count * sizeof(uint32_t));
    if (!status) {
        cout << "receiveMessage: Unable to read handles: " << status << endl;
        return status;
    }

    vector<shared_ptr<Port>> ports(count, status);
    if (!status) {
        cout << "receiveMessage: Unable to create vector of " << count << " Port pointers: "
             << status << endl;
        return status;
    }

    for (size_t index = 0; index < count; index++) {
        Result<shared_ptr<Port>> result = process->handleManager.lookupPort(portHandles[index]);
        if (!result) {
            cout << "receiveMessage: invalid port handle " << portHandles[index] << endl;
            return result.status();
        }

        ports[index] = *result;
    }

    status = mergeSort(ports);
    if (!status) {
        cout << "receiveMessage: unable to sort ports: " << status << endl;
        return status;
    }

    /* Danger: non-RAII locking */
    for (auto &port: ports) {
        port->lock.lock();
    }

    size_t resultIndex = 0;

    Result<unique_ptr<Message>> messageResult;
    for (size_t index = 0; index < ports.size(); index++) {
        messageResult = ports[index]->getMessage();
        if (!messageResult) {
            /* error status */
            goto unlockAndReturn;
        }
        /* break if we found a message */
        if (*messageResult) {
            resultIndex = index;
            break;
        }
    }

    /* no pending message, Thread has to wait */
    if (!*messageResult) {
        /* Danger Zone: Asymmetric lock/unlock: These two locks will not be unlocked once this
         * method leaves, but once the thread state is discarded. */
        KernelInterruptsLock.lock();
        Processor *processor = CurrentProcessor();
        processor->scheduler.lock.lock();

        Thread *thread = CurrentThread();
        processor->scheduler.removeActiveThread();
        thread->setState(Thread::State::WaitMessage());

        for (size_t index = 0; index < ports.size(); index++) {
            ports[index]->setWaitingThread(thread, index);
        }

        messageResult = Status::DiscardStateAndSchedule();
    }

unlockAndReturn:
    for (auto &port: ports) {
        port->lock.unlock();
    }

    if (messageResult) {
        cout << "ReceiveMessage resultIndex: " << resultIndex << endl;
        /* Transfer the resultIndex variable into the message info */
        Status status = process->addressSpace.copyTo((*messageResult)->infoAddress.value(),
                                                     reinterpret_cast<uint8_t *>(&resultIndex),
                                                     sizeof(size_t),
                                                     true);
        if (status) {
            return (*messageResult)->infoAddress.value();
        } else {
            return status;
        }
    } else {
        return messageResult.status();
    }
}
