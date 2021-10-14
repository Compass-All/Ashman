#ifndef ASHMAN_MEMPOOL_H
#define ASHMAN_MEMPOOL_H

#define ROUND_UP(a, b) ((((a)-1) / (b) + 1) * (b))
#define ROUND_DOWN(a, b) ((a) / (b) * (b))

#define PARTITION_SHIFT 23
#define PARTITION_SIZE (1UL << PARTITION_SHIFT)

#define PARTITION_UP(addr) (ROUND_UP(addr, PARTITION_SIZE))
#define PARTITION_DOWN(addr) ((addr) & (~((PARTITION_SIZE)-1)))

#define MEMORY_POOL_START 0x140000000
#define MEMORY_POOL_END 0x170000000
#define MEMORY_POOL_PARTITION_NUM \
    ((MEMORY_POOL_END - MEMORY_POOL_START) >> PARTITION_SHIFT)

#ifndef __ASSEMBLER__

#include <enclave.h>
#include <stdint.h>

typedef struct __partition {
    uintptr_t pfn;
    int owner;
    uintptr_t va;
} partition_t;

extern partition_t memory_pool[MEMORY_POOL_PARTITION_NUM];

void init_memory_pool(void);

uintptr_t alloc_partition(enclave_context_t* ectx, uintptr_t va);

uintptr_t free_partition(int eid);

int partition_migration(uintptr_t src_pfn, uintptr_t dst_pfn);

#endif // !__ASSEMBLER__

#endif // !ASHMAN_MEMPOOL_H