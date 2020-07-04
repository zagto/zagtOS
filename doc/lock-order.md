Process::pagingLock
FutexManager::lock
Scheduler::lock

after owner:
Thread::stateLock (only do one at a time - a scheduler may lock one second one)
