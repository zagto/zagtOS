#include <processes/Thread.hpp>
#include <processes/Process.hpp>
#include <system/System.hpp>


Thread::~Thread() {
    assert(currentProcessor == CurrentProcessor);
    /* don't try deleting special threads */
    assert(process);

    CurrentProcessor->scheduler.remove(this);
    cout << "removed from scheduler" << endl;
    process->removeThread(this);
}
