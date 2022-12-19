#include <log/BasicLog.hpp>
#include <log/SerialBackend.hpp>
#include <log/FramebufferBackend.hpp>
#include <system/System.hpp>
#include <system/Processor.hpp>
namespace basicLog {

// TODO: globals break global constructor ordering
static SerialBackend serialBackend;
static FramebufferBackend framebufferBackend;

static SpinLock logLock;

void init() {
    assert(!ProcessorsInitialized);
    serialBackend.init();
    framebufferBackend.init(_HandOverSystem->framebufferInfo);
    write(KERNEL_COLOR);
}

static void output(char character) {
    serialBackend.write(character);
    framebufferBackend.write(character);
}

static void flush() {
    scoped_lock lg1(KernelInterruptsLock);
    scoped_lock lg(logLock);

    /* TODO: we may want to do this in a way the flusch happens on the same processor as write */

    char *buffer = CurrentProcessor()->logBuffer;
    for (size_t i = 0; i < CurrentProcessor()->logBufferIndex; i++) {
        output(buffer[i]);
    }
    CurrentProcessor()->logBufferIndex = 0;
}

void write(char character) {
    if (!ProcessorsInitialized) {
        scoped_lock lg1(KernelInterruptsLock);
        scoped_lock lg(logLock);
        output(character);
    } else {
        char *buffer = CurrentProcessor()->logBuffer;
        size_t &index = CurrentProcessor()->logBufferIndex;

        buffer[index] = character;
        index++;

        if (character == '\n' || index == Processor::LOG_BUFFER_SIZE) {
            flush();
        }
    }
}

void sendCoreDump(size_t nameLength,
                  const uint8_t *name,
                  size_t dataLength,
                  const uint8_t *data) {
    cout << "sending coredump data size " << dataLength << endl;
    flush();

    scoped_lock lg1(KernelInterruptsLock);
    scoped_lock lg(logLock);
    /* core dump marker */
    serialBackend.writeCharacter(static_cast<char>(0xf2));
    for (size_t i = 0; i < 8; i++) {
        serialBackend.writeCharacter((nameLength >> i * 8) & 0xff);
    }
    for (size_t i = 0; i < nameLength; i++) {
        serialBackend.writeCharacter(name[i]);
    }
    for (size_t i = 0; i < 8; i++) {
        serialBackend.writeCharacter((dataLength >> i * 8) & 0xff);
    }
    for (size_t i = 0; i < dataLength; i++) {
        serialBackend.writeCharacter(data[i]);
    }
}

}
