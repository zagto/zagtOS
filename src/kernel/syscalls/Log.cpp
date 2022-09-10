#include <syscalls/Log.hpp>
#include <processes/Process.hpp>
#include <log/BasicLog.hpp>

size_t Log(const shared_ptr<Process> &process,
                   size_t _address,
                   size_t _length,
                   size_t,
                   size_t,
                   size_t) {
    static const size_t MAX_LOG_SIZE = 10000;
    size_t address = _address;
    size_t length = _length;

    if (length > MAX_LOG_SIZE) {
        // TODO
        cout << "Process attempted to send huge log. ignoring.\n";
        return 0;
    }

    /* zero-size log */
    if (length == 0) {
        return 0;
    }

    vector<uint8_t> buffer(length);
    process->addressSpace.copyFrom(&buffer[0], address, length);
    /* do not print program name for small invisible stuff */
    if (!(length == 0 || (length == 1 && buffer[0] <= ' '))) {
        basicLog::write(basicLog::PROGRAM_NAME_COLOR);
        for (uint8_t character: process->logName) {
            cout << static_cast<char>(character);
        }
        basicLog::write(basicLog::PROGRAM_COLOR);
        cout << ": ";
    } else {
        basicLog::write(basicLog::PROGRAM_COLOR);
    }
    for (uint8_t character: buffer) {
        cout << static_cast<char>(character);
    }
    basicLog::write(basicLog::KERNEL_COLOR);
    return 0;
}
