#pragma once

template<typename T> class shared_ptr;
template<typename T> class weak_ptr;
class Thread;
class RegisterState;

bool Futex(Thread *thread, RegisterState &registerState);
