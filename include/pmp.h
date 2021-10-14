#ifndef ASHMAN_PMP_H
#define ASHMAN_PMP_H

#define PMP_REGION_NUM 8

#ifndef __ASSEMBLER__

#include <stdint.h>

typedef struct __pmp_region {
    uintptr_t pmp_start;
    uintptr_t pmp_size;
    uintptr_t used;
} pmp_region_t;

#endif // !__ASSEMBLER__

#endif // !ASHMAN_PMP_H