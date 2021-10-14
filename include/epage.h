#ifndef ASHMAN_EPAGE_H
#define ASHMAN_EPAGE_H

/* SV39 Page */
#define SATP_MODE_SV39 8UL
#define SATP_MODE_SHIFT 60
#define EPAGE_SHIFT 12
#define EPAGE_SIZE (1 << EPAGE_SHIFT)

#define MASK_OFFSET 0xfff
#define MASK_L0 0x1ff000
#define MASK_L1 0x3fe00000
#define MASK_L2 0x7fc0000000

#define INVERSE_MAP_ENTRY_NUM 1024

#ifndef __ASSEMBLER__

#include <stddef.h>
#include <stdint.h>

typedef struct __inv_map {
    uintptr_t pa;
    uintptr_t va;
    uint32_t n;
} inverse_map_t;

typedef struct __pte {
    uint32_t pte_v : 1;
    uint32_t pte_r : 1;
    uint32_t pte_w : 1;
    uint32_t pte_x : 1;
    uint32_t pte_u : 1;
    uint32_t pte_g : 1;
    uint32_t pte_a : 1;
    uint32_t pte_d : 1;
    uint32_t rsw : 2;
    uintptr_t ppn : 44;
    uintptr_t __unused_value : 10;
} pte_t;

void update_tree_pte(uintptr_t root, uintptr_t pa_diff);

void update_leaf_pte(uintptr_t root, uintptr_t va, uintptr_t pa);

inverse_map_t* look_up_inverse_map(inverse_map_t* inv_map, uintptr_t pa);

#endif // !__ASSEMBLER__

#endif // !ASHMAN_EPAGE_H