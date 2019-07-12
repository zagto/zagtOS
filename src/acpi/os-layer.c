#define _GNU_SOURCE 1
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <pthread.h>
#include <stdarg.h>
#include <stdio.h>
#include <unistd.h>
#include <assert.h>
#include <time.h>
#include <semaphore.h>
#include <semaphore.h>
#include <zagtos/acpi.h>
#include <zagtos/memory.h>
#include <zagtos/interrupt.h>
#include <acpi.h>
#include <pci.h>


/* ACPIA OS Layer
 * For information, see: https://wiki.osdev.org/ACPICA
 */


/*
 * Environmental and ACPI Tables
 */
ACPI_STATUS AcpiOsTerminate(void) {
    return AE_OK;
}

ACPI_STATUS AcpiOsInitialize(void) {
    return AE_OK;
}

ACPI_PHYSICAL_ADDRESS AcpiOsGetRootPointer(void) {
    return zagtos_get_acpi_root();
}

ACPI_STATUS AcpiOsPredefinedOverride(const ACPI_PREDEFINED_NAMES *PredefinedObject,
                                     ACPI_STRING *NewValue) {
    *NewValue = NULL;
    return AE_OK;
}


ACPI_STATUS AcpiOsTableOverride(ACPI_TABLE_HEADER *ExistingTable,
                                ACPI_TABLE_HEADER **NewTable) {
    *NewTable = NULL;
    return AE_OK;
}

ACPI_STATUS AcpiOsPhysicalTableOverride(ACPI_TABLE_HEADER *ExistingTable,
                                        ACPI_PHYSICAL_ADDRESS *NewAddress,
                                        UINT32 *NewTableLength) {
    *NewAddress = 0;
    return AE_OK;
}

ACPI_STATUS AcpiOsEnterSleep(UINT8 SleepState, UINT32 RegaValue, UINT32 RegbValue) {
    return AE_OK;
}


/*
 * Memory Management
 */
void *AcpiOsMapMemory(ACPI_PHYSICAL_ADDRESS PhysicalAddress, ACPI_SIZE Length) {
    return zagtos_map_physical_area(PhysicalAddress, Length);
}

void AcpiOsUnmapMemory(void *where, ACPI_SIZE length) {
    zagtos_unmap_physical_area(where, length);
}

ACPI_STATUS AcpiOsGetPhysicalAddress(void *LogicalAddress,
                                     ACPI_PHYSICAL_ADDRESS *PhysicalAddress) {
    *PhysicalAddress = zagtos_get_physical_address(LogicalAddress);
    return AE_OK;
}

void *AcpiOsAllocate(ACPI_SIZE Size) {
    return malloc(Size);
}

void AcpiOsFree(void *Memory) {
    free(Memory);
}

/*BOOLEAN checkMemoryRegion(void *pointer, size_t length) {
    // this should really never be used
    return TRUE;
}


BOOLEAN AcpiOsReadable(void *Memory, ACPI_SIZE Length) {
    return checkMemoryRegion(Memory, Length);
}

BOOLEAN AcpiOsWritable(void *Memory, ACPI_SIZE Length) {
    return checkMemoryRegion(Memory, Length);
}*/

/*
 * Multithreading and Scheduling Services
 */
ACPI_THREAD_ID AcpiOsGetThreadId() {
    return (ACPI_THREAD_ID)pthread_self();
}

static sem_t events_semaphore;
static bool events_semaphore_initialized = false;

ACPI_STATUS AcpiOsExecute(ACPI_EXECUTE_TYPE Type, ACPI_OSD_EXEC_CALLBACK function, void *context) {
    pthread_t thread;

    if (!events_semaphore_initialized) {
        sem_init(&events_semaphore, 0, 0);
        events_semaphore_initialized = true;
    }
    sem_post(&events_semaphore);

    pthread_create(&thread, NULL, (void *(*)(void *))(size_t)function, context);
    return AE_OK;
}

void AcpiOsSleep(UINT64 Milliseconds) {
    struct timespec ts;
    ts.tv_sec = Milliseconds / 1000;
    ts.tv_nsec = Milliseconds * 1000 * 1000;
    nanosleep(&ts, NULL);
}

void AcpiOsStall(UINT32 Microseconds) {
    struct timespec target, ts;
    timespec_get(&target, TIME_UTC);

    uint64_t nsecs = (uint64_t)Microseconds * 1000 + target.tv_nsec;
    target.tv_sec += nsecs / 1000000000;
    target.tv_nsec = nsecs % 1000000000;

    do {
        timespec_get(&ts, TIME_UTC);
    } while (ts.tv_sec < target.tv_sec
             || (ts.tv_sec == target.tv_sec && ts.tv_nsec < target.tv_nsec));
}

void AcpiOsWaitEventsComplete(void) {
    sem_wait(&events_semaphore);
}

ACPI_STATUS AcpiOsCreateMutex(ACPI_MUTEX *OutHandle) {
    pthread_mutex_t *mutex = malloc(sizeof(pthread_mutex_t));
    pthread_mutex_init(mutex, NULL);
    *OutHandle = (ACPI_MUTEX)mutex;
    return  AE_OK;
}

void AcpiOsDeleteMutex(ACPI_MUTEX Handle) {
    pthread_mutex_t *mutex = (pthread_mutex_t *)Handle;
    pthread_mutex_destroy(mutex);
    free(mutex);
}

ACPI_STATUS AcpiOsAcquireMutex(ACPI_MUTEX Handle, UINT16 Timeout) {
    pthread_mutex_t *mutex = (pthread_mutex_t *)Handle;
    if (Timeout == 0xffff) {
        pthread_mutex_lock(mutex);
        return AE_OK;
    } else if (Timeout == 0) {
        if (pthread_mutex_trylock(mutex)) {
            return AE_TIME;
        } else {
            return AE_OK;
        }
    } else {
        struct timespec target;
        timespec_get(&target, TIME_UTC);

        uint64_t nsecs = (uint64_t)Timeout * 1000000 + target.tv_nsec;
        target.tv_sec += nsecs / 1000000000;
        target.tv_nsec = nsecs % 1000000000;

        if (pthread_mutex_timedlock(mutex, &target)) {
            return AE_TIME;
        } else {
            return AE_OK;
        }
    }
}

void AcpiOsReleaseMutex(ACPI_MUTEX Handle) {
    pthread_mutex_unlock((pthread_mutex_t *)Handle);
}

ACPI_STATUS AcpiOsCreateSemaphore(UINT32 MaxUnits, UINT32 InitialUnits, ACPI_SEMAPHORE *OutHandle) {
    sem_t *sem = malloc(sizeof(sem_t));
    sem_init(sem, 0, InitialUnits);
    *OutHandle = (ACPI_SEMAPHORE)sem;
    return AE_OK;
}

ACPI_STATUS AcpiOsDeleteSemaphore(ACPI_SEMAPHORE Handle) {
    sem_t *sem = (sem_t *)Handle;
    sem_destroy(sem);
    free(sem);
    return AE_OK;
}

ACPI_STATUS AcpiOsWaitSemaphore(ACPI_SEMAPHORE Handle, UINT32 Units, UINT16 Timeout) {
    /*
     * WARNING:
     *
     *  This does not really support multiple units
     */
    if (Units != 1) {
        printf("ACPI: AcpiOsWaitSemaphore called with %u units\n", Units);
    }

    if (Timeout == 0xffff) {
        for (uint32_t i = 0; i < Units; i++) {
            sem_wait((sem_t *)Handle);
        }
        return AE_OK;
    } else {
        struct timespec target;
        timespec_get(&target, TIME_UTC);

        uint64_t nsecs = (uint64_t)Timeout * 1000000 + target.tv_nsec;
        target.tv_sec += nsecs / 1000000000;
        target.tv_nsec = nsecs % 1000000000;


        for (uint32_t i = 0; i < Units; i++) {
            if (sem_timedwait((sem_t *)Handle, &target)) {
                return AE_TIME;
            }
        }
        return AE_OK;
    }
}

ACPI_STATUS AcpiOsSignalSemaphore(ACPI_SEMAPHORE Handle, UINT32 Units) {
    for (uint32_t i = 0; i < Units; i++) {
        sem_post((sem_t *)Handle);
    }
    return AE_OK;
}

ACPI_STATUS AcpiOsCreateLock(ACPI_SPINLOCK *OutHandle) {
    return AcpiOsCreateMutex(OutHandle);
}

void AcpiOsDeleteLock(ACPI_HANDLE Handle) {
    AcpiOsDeleteMutex(Handle);
}

ACPI_CPU_FLAGS AcpiOsAcquireLock(ACPI_SPINLOCK Handle) {
    AcpiOsAcquireMutex(Handle, 0xffff);
    return 0;
}

void AcpiOsReleaseLock(ACPI_SPINLOCK Handle, ACPI_CPU_FLAGS Flags) {
    AcpiOsReleaseMutex(Handle);
}



/*
 * Memory Access and Memory Mapped I/O
 */
ACPI_STATUS AcpiOsReadMemory(ACPI_PHYSICAL_ADDRESS Address, UINT64 *Value, UINT32 Width) {
    assert(Width % 8 == 0 && Width <= 64);
    void *pointer = zagtos_map_physical_area(Address, Width / 8);
    switch(Width) {
    case 8:
        *Value = *(uint8_t *)pointer;
        break;
    case 16:
        *Value = *(uint16_t *)pointer;
        break;
    case 32:
        *Value = *(uint32_t *)pointer;
        break;
    case 64:
        *Value = *(uint64_t *)pointer;
        break;
    }
    zagtos_unmap_physical_area(pointer, Width / 8);
    return AE_OK;
}

ACPI_STATUS AcpiOsWriteMemory(ACPI_PHYSICAL_ADDRESS Address, UINT64 Value, UINT32 Width) {
    assert(Width % 8 == 0 && Width <= 64);
    void *pointer = zagtos_map_physical_area(Address, Width / 8);
    switch(Width) {
    case 8:
        *(uint8_t *)pointer = Value;
        break;
    case 16:
        *(uint16_t *)pointer = Value;
        break;
    case 32:
        *(uint32_t *)pointer = Value;
        break;
    case 64:
        *(uint64_t *)pointer = Value;
        break;
    }
    zagtos_unmap_physical_area(pointer, Width / 8);
    return AE_OK;
}

ACPI_STATUS AcpiOsReadPort(ACPI_IO_ADDRESS Address, UINT32 *Value, UINT32 Width) {
    assert(Width % 8 == 0 && Width <= 32);
    *Value = zagtos_io_port_read(Address, Width / 8);
    return AE_OK;
}

ACPI_STATUS AcpiOsWritePort(ACPI_IO_ADDRESS Address, UINT32 Value, UINT32 Width) {
    assert(Width % 8 == 0 && Width <= 32);
    zagtos_io_port_write(Address, Width / 8, Value);
    return AE_OK;
}


/*
 * PCI Configuration Space Access
 */
ACPI_STATUS AcpiOsReadPciConfiguration(ACPI_PCI_ID *PciId,
                                       UINT32 Reg,
                                       UINT64 *Value,
                                       UINT32 Width) {
    size_t base = pci_get_config_address(PciId);
    return AcpiOsReadMemory(base + Reg, Value, Width);
}

ACPI_STATUS AcpiOsWritePciConfiguration(ACPI_PCI_ID *PciId,
                                        UINT32 Reg,
                                        UINT64 Value,
                                        UINT32 Width) {
    size_t base = pci_get_config_address(PciId);
    return AcpiOsWriteMemory(base + Reg, Value, Width);
}


/*
 * Interrupt Handling
 */
struct handler {
    uint32_t interrupt;
    ACPI_OSD_HANDLER callback;
    void *data;
};

static struct handler *handlers = NULL;
static size_t numHandlers = 0;
static pthread_mutex_t handlersMutex = PTHREAD_MUTEX_INITIALIZER;

size_t findHandler(uint32_t interrupt) {
    for (size_t i = 0; i < numHandlers; i++) {
        if (handlers[i].interrupt == interrupt) {
            return i;
        }
    }
    printf("ACPI: tried to find non existing interrupt handler for %i\n", interrupt);
    exit(1);
}

UINT32 AcpiOsInstallInterruptHandler(UINT32 InterruptNumber, ACPI_OSD_HANDLER ServiceRoutine, void *Context)
{
    pthread_mutex_lock(&handlersMutex);

    numHandlers++;
    if (handlers) {
        handlers = realloc(handlers, numHandlers * sizeof(struct handler));
    } else {
        handlers = malloc(sizeof(struct handler *));
    }
    struct handler *h = &handlers[numHandlers - 1];

    h->interrupt = InterruptNumber;
    h->callback = ServiceRoutine;
    h->data = Context;

    zagtos_register_interrupt(InterruptNumber);
    pthread_mutex_unlock(&handlersMutex);
    return AE_OK;
}

ACPI_STATUS AcpiOsRemoveInterruptHandler(UINT32 InterruptNumber, ACPI_OSD_HANDLER Handler) {
    pthread_mutex_lock(&handlersMutex);

    zagtos_unregister_interrupt(InterruptNumber);

    size_t index = findHandler(InterruptNumber);
    numHandlers--;

    handlers[index] = handlers[numHandlers];
    handlers = realloc(handlers, numHandlers * sizeof(struct handler));

    pthread_mutex_unlock(&handlersMutex);
    return AE_OK;
}


/*
 * Formatted Output
 */
void ACPI_INTERNAL_VAR_XFACE AcpiOsPrintf(const char *format, ...) {
    va_list args;
    vprintf(format, args);
}

void AcpiOsVprintf(const char *format, va_list args) {
    vprintf(format, args);
}


/*
 * Miscellaneous
 */
UINT64 AcpiOsGetTimer(void) {
    struct timespec ts;
    timespec_get(&ts, TIME_UTC);
    return (uint64_t)ts.tv_sec * 10000000 + (uint64_t)ts.tv_nsec / 100;
}

ACPI_STATUS AcpiOsSignal(UINT32 Function, void *Info) {
    printf("Got AcpiOsSignal. Terminating\n");
    exit(1);
}
