#include <common/common.hpp>
#include <memory>
#include <processes/Thread.hpp>
#include <interrupts/RegisterState.hpp>
#include <memory/UserSpaceObject.hpp>
#include <processes/FutexManager.hpp>
#include <system/System.hpp>


static constexpr uint32_t FUTEX_WAIT = 0,
                          FUTEX_WAKE = 1,
                          FUTEX_FD = 2,
                          FUTEX_PRIVATE = 128;


bool Futex(shared_ptr<Thread> thread, const RegisterState &registerState) {
    size_t address = registerState.syscallParameter(0);
    uint32_t operation = registerState.syscallParameter(1);
    UserSpaceObject<timespec, USOOperation::READ> timeout(registerState.syscallParameter(2),
                                                          thread->process);

    FutexManager &manager = (operation & FUTEX_PRIVATE) ? CurrentSystem.futexManager : thread->process->futexManager;

    switch (operation) {
    case FUTEX_WAIT:


    }
}
