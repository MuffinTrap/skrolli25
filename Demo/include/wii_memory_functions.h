#ifndef WII_MEMORY_FUNCTIONS_H
#define WII_MEMORY_FUNCTIONS_H

void* AllocateGPUMemory(size_t size);
void FlushGPUCache(void* buffer, size_t size);

#endif
