#include <tasks/Thread.hpp>
#include <tasks/Task.hpp>
#include <system/System.hpp>


Thread::~Thread() {
    assert(currentProcessor == CurrentProcessor);
    /* don't try deleting special threads */
    assert(task);

    CurrentProcessor->scheduler.remove(this);
    cout << "removed from scheduler" << endl;
    task->removeThread(this);
}
