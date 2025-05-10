#ifndef _GNU_SOURCE
#   define _GNU_SOURCE
#endif

#include "VirtualMemory.h"
#include <stdint.h>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

void *beacon_virtualMemory_allocateSystemMemory(size_t sizeToAllocate)
{
    return VirtualAlloc(NULL, sizeToAllocate, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
}

void beacon_virtualMemory_freeSystemMemory(void *memory, size_t sizeToFree)
{
    (void)sizeToFree;
    VirtualFree(memory, 0, MEM_RELEASE);
}

size_t beacon_virtualMemory_getSystemAllocationAlignment(void)
{
    SYSTEM_INFO systemInfo;
    memset(&systemInfo, 0, sizeof(systemInfo));
    GetSystemInfo(&systemInfo);
    return systemInfo.dwPageSize;
}

bool beacon_virtualMemory_lockCodePagesForWriting(void *writePointer, void *executablePointer, size_t size)
{
    if(writePointer != executablePointer)
        return true;

    size_t pageAlignment = beacon_virtualMemory_getSystemAllocationAlignment();
    uintptr_t startAddress = (uintptr_t)executablePointer & (-pageAlignment);
    uintptr_t endAddress = ((uintptr_t)executablePointer + size + pageAlignment - 1) & (-pageAlignment);

    DWORD oldProtection = 0;
    return VirtualProtect((void*)startAddress, endAddress - startAddress, PAGE_EXECUTE_READWRITE, &oldProtection);
}

bool beacon_virtualMemory_unlockCodePagesForExecution(void *writePointer, void *executablePointer, size_t size)
{
    if(writePointer != executablePointer)
        return;

    size_t pageAlignment = beacon_virtualMemory_getSystemAllocationAlignment();
    uintptr_t startAddress = (uintptr_t)executablePointer & (-pageAlignment);
    uintptr_t endAddress = ((uintptr_t)executablePointer + size + pageAlignment - 1) & (-pageAlignment);

    DWORD oldProtection = 0;
    VirtualProtect((void*)startAddress, endAddress - startAddress, PAGE_EXECUTE_READ, &oldProtection);
}

bool beacon_virtualMemory_hasSupportForRWXMapping(void)
{
    return true;
}

void *beacon_virtualMemory_allocateSystemMemoryWithDualMapping(size_t sizeToAllocate, void **writeableMapping, void **executableMapping)
{
    *writeableMapping = NULL;
    *executableMapping = NULL;
    return NULL;
}

void beacon_virtualMemory_freeSystemMemoryWithDualMapping(size_t sizeToFree, void *mappingHandle, void *writeableMapping, void *executableMapping)
{
}

#else

#include <sys/mman.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

static pthread_mutex_t codeZoneWriteLock = PTHREAD_MUTEX_INITIALIZER;

bool beacon_virtualMemory_hasSupportForRWXMapping(void)
{
    static bool hasCachedResult = false;
    static bool cachedResult;

    if(!hasCachedResult)
    {
        size_t pageSize = getpagesize();
        void * result = mmap(0, pageSize, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_PRIVATE | MAP_ANON, -1, 0);
        cachedResult = result != MAP_FAILED;
        if(result != MAP_FAILED)
            munmap(result, pageSize);

        hasCachedResult = true;
    }

    return cachedResult;
}

void *beacon_virtualMemory_allocateSystemMemory(size_t sizeToAllocate)
{
    void *result = mmap(0, sizeToAllocate, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON, -1, 0);
    if(result == MAP_FAILED)
        return 0;

    return result;
}

void *beacon_virtualMemory_allocateSystemMemoryWithDualMapping(size_t sizeToAllocate, void **writeableMapping, void **executableMapping)
{
    *writeableMapping = NULL;
    *executableMapping = NULL;

    int fd = memfd_create("beacon_code", MFD_CLOEXEC);
    if(fd < 0)
    {
        perror("failed to make memfd.");
        abort();
    }

    if(ftruncate(fd, sizeToAllocate) < 0)
    {
        perror("failed to allocate memory with dual mapping for JIT execution.");
        abort();
    }

    // Read-Write mapping.
    void *mmapResult = mmap(0, sizeToAllocate, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if(mmapResult == MAP_FAILED)
    {
        perror("failed to map read-write memory for JIT execution.");
        abort();
    }
    *writeableMapping = mmapResult;

    // Read-Execute mapping.
    mmapResult = mmap(0, sizeToAllocate, PROT_READ | PROT_EXEC, MAP_SHARED, fd, 0);
    if(mmapResult == MAP_FAILED)
    {
        perror("failed to map read-write memory for JIT execution.");
        abort();
    }
    *executableMapping = mmapResult;

    return (void*)(intptr_t)fd;
}

void beacon_virtualMemory_freeSystemMemoryWithDualMapping(size_t sizeToFree, void *mappingHandle, void *writeableMapping, void *executableMapping)
{
    int fd = (intptr_t)mappingHandle;
    if(fd < 0)
        return;

    munmap(writeableMapping, sizeToFree);
    munmap(executableMapping, sizeToFree);
    close(fd);
}

void beacon_virtualMemory_freeSystemMemory(void *memory, size_t sizeToFree)
{
    munmap(memory, sizeToFree);
}

size_t beacon_virtualMemory_getSystemAllocationAlignment(void)
{
    return getpagesize();
}

bool beacon_virtualMemory_lockCodePagesForWriting(void *writePointer, void *executablePointer, size_t size)
{
    if(writePointer != executablePointer)
        return true;

    size_t pageAlignment = beacon_virtualMemory_getSystemAllocationAlignment();
    uintptr_t startAddress = (uintptr_t)executablePointer & (-pageAlignment);
    uintptr_t endAddress = ((uintptr_t)executablePointer + size + pageAlignment - 1) & (-pageAlignment);

    pthread_mutex_lock(&codeZoneWriteLock);
    bool success = mprotect((void*)startAddress, endAddress - startAddress, PROT_READ | PROT_WRITE | PROT_EXEC) == 0;
    if(!success)
        pthread_mutex_unlock(&codeZoneWriteLock);

    return success;
}

bool beacon_virtualMemory_unlockCodePagesForExecution(void *writePointer, void *executablePointer, size_t size)
{
    if(writePointer != executablePointer)
        return true;

    size_t pageAlignment = beacon_virtualMemory_getSystemAllocationAlignment();
    uintptr_t startAddress = (uintptr_t)executablePointer & (-pageAlignment);
    uintptr_t endAddress = ((uintptr_t)executablePointer + size + pageAlignment - 1) & (-pageAlignment);

    bool success = mprotect((void*)startAddress, endAddress - startAddress, PROT_READ | PROT_EXEC) == 0;
    pthread_mutex_unlock(&codeZoneWriteLock);
    return success;
}

#endif