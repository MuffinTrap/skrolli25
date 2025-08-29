#include <stdlib.h>

#ifdef GEKKO
#   include <gccore.h>
#endif

void* AllocateGPUMemory(size_t size)
{
#ifdef GEKKO
	return aligned_alloc(32, size);
#else
    return malloc(size);
#endif

}
void FlushGPUCache(void* buffer, size_t size)
{
#ifdef GEKKO
    DCFlushRange(buffer, size);
#else
    // NOP
#endif
}
