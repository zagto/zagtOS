#include <memory>
#include <processes/Process.hpp>

/* all shared_ptr/weak_ptr types need to be instanciated here explicitly. This allowes these simple
 * delete calls here. They have been moved out from the header so shared_ptr<Class> types can be
 * used in a header with incomplete type Class. This is not how a shared_ptr implementation from an
 * actual C++ standard library works. */
template class shared_ptr<Process>;
template class shared_ptr<KernelStack>;
template class shared_ptr<Thread>;
template class shared_ptr<Port>;
template class shared_ptr<MappedArea>;
template class shared_ptr<MemoryArea>;
template class shared_ptr<BoundInterrupt>;
template class shared_ptr<EventQueue>;

template<typename T>
shared_ptr<T>::InternalPointer::~InternalPointer() noexcept {
    if (rawPointer) {
        delete rawPointer;
    }
}

template<typename T>
void shared_ptr<T>::InternalPointer::checkReferenceCounts(bool &shouldDelete) noexcept {
    if (referenceCount == 0) {
        if (weakReferenceCount == 0) {
            shouldDelete = true;
        } else {
            delete rawPointer;
            valid = false;
        }
    }
}
