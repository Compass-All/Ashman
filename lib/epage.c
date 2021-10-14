#include <epage.h>

#define PTE_CHECK(pte, bit) ((pte)->pte_##bit)

#define is_leaf_pte(pte) (PTE_CHECK(pte, r) || PTE_CHECK(pte, w) || PTE_CHECK(pte, x))

static pte_t* get_pte(pte_t* root, uintptr_t va)
{
    uintptr_t layer_offset[] = { (va & MASK_L2) >> 30, (va & MASK_L1) >> 21,
        (va & MASK_L0) >> 12 };
    pte_t* tmp_entry;
    int i;
    for (i = 0; i < 3; ++i) {
        tmp_entry = &root[layer_offset[i]];
        if (!PTE_CHECK(tmp_entry, v)) {
            return NULL;
        }
        if (is_leaf_pte(tmp_entry)) {
            return tmp_entry;
        }
        root = (pte_t*)((uintptr_t)tmp_entry->ppn << EPAGE_SHIFT);
    }
    return NULL;
}

void update_tree_pte(uintptr_t root, uintptr_t pa_diff)
{
    pte_t* entry = (pte_t*)root;
    uintptr_t next_level;
    int i;

    for (i = 0; i < 512; ++i, ++entry) {
        if (is_leaf_pte(entry)) {
            return;
        }
        if (!PTE_CHECK(entry, v)) {
            continue;
        }

        entry->ppn += pa_diff >> EPAGE_SHIFT;
        next_level = (uintptr_t)entry->ppn << EPAGE_SHIFT;
        update_tree_pte(next_level, pa_diff);
    }
}

void update_leaf_pte(uintptr_t root, uintptr_t va, uintptr_t pa)
{
    pte_t* entry = get_pte((pte_t*)root, va);
    if (entry) {
        entry->ppn = pa >> EPAGE_SHIFT;
    }
}

inverse_map_t* look_up_inverse_map(inverse_map_t* inv_map, uintptr_t pa)
{
    for (int i = 0; inv_map[i].pa && i < INVERSE_MAP_ENTRY_NUM; i++) {
        if (inv_map[i].pa == pa)
            return &inv_map[i];
    }

    return NULL;
}