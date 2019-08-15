/* signals are not supported. This file is here to make libstdc++ build. */
typedef int sig_atomic_t;
void signal();
void raise();
