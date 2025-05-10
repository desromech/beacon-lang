#ifndef BEACON_INTERNAL_VIRTUAL_MEMORY_H
#define BEACON_INTERNAL_VIRTUAL_MEMORY_H

#pragma once

#include <stddef.h>
#include <stdbool.h>

bool beacon_virtualMemory_hasSupportForRWXMapping(void);

void *beacon_virtualMemory_allocateSystemMemory(size_t sizeToAllocate);
void beacon_virtualMemory_freeSystemMemory(void *memory, size_t sizeToFree);

size_t beacon_virtualMemory_getSystemAllocationAlignment(void);

void *beacon_virtualMemory_allocateSystemMemoryWithDualMapping(size_t sizeToAllocate, void **writeableMapping, void **executableMapping);
void beacon_virtualMemory_freeSystemMemoryWithDualMapping(size_t sizeToFree, void *mappingHandle, void *writeableMapping, void *executableMapping);

bool beacon_virtualMemory_lockCodePagesForWriting(void *writePointer, void *executablePointer, size_t size);
bool beacon_virtualMemory_unlockCodePagesForExecution(void *writePointer, void *executablePointer, size_t size);

#endif //BEACON_INTERNAL_VIRTUAL_MEMORY_H
