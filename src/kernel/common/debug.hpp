#pragma once

template <typename T>
class shared_ptr;
class Process;
__attribute__((noreturn)) void enterDebugger(shared_ptr<Process> process);
