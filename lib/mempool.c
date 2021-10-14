#include <epage.h>
#include <mempool.h>
#include <stddef.h>

typedef struct __region {
    uintptr_t pfn;
    size_t length;
} region_t;

#define for_each_partition_in_pool(pool, partition, i)               \
    for (i = 0, partition = &pool[i]; i < MEMORY_POOL_PARTITION_NUM; \
         i++, partition = &pool[i])

#define for_each_partition_in_pool_rev(pool, partition, i)                \
    for (i = MEMORY_POOL_PARTITION_NUM - 1, partition = &pool[i]; i >= 0; \
         i--, partition = &pool[i])

#define pfn_to_partition(pfn) \
    &memory_pool[((pfn) - (MEMORY_POOL_START >> PARTITION_SHIFT))]

#define die_oom() \
    do {          \
    } while (1)

partition_t memory_pool[MEMORY_POOL_PARTITION_NUM];

static region_t find_largest_avail()
{
    int i;
    partition_t* ptn;
    uintptr_t head = 0, tail = 0;
    int hit = 0;
    int max_len = 0;
    int len;
    region_t ret = { 0 };

    for_each_partition_in_pool(memory_pool, ptn, i)
    {
        if (ptn->owner >= 0 && hit) {
            len = (int)(tail - head + 1);
            if (len > max_len) {
                max_len = len;
                ret.pfn = head;
                ret.length = max_len;
            }
            hit = 0;
        }

        if (ptn->owner < 0 && !hit) {
            head = ptn->pfn;
            tail = ptn->pfn;
            hit = 1;
            continue;
        }

        if (ptn->owner < 0 && hit) {
            tail++;
        }
    }

    if (hit) {
        len = (int)(tail - head + 1);
        if (len > max_len) {
            max_len = len;
            ret.pfn = head;
            ret.length = max_len;
        }
    }

    return ret;
}

static partition_t* find_available_partition(void)
{
    uintptr_t ret_pfn = 0;
    partition_t* sec = NULL;
    region_t reg = find_largest_avail();

    if (reg.length == 0) {
        return NULL;
    }

    ret_pfn = reg.pfn + (reg.length >> 1);
    sec = pfn_to_partition(ret_pfn);

    return sec;
}

static void update_partition_info(uintptr_t pfn, int owner, uintptr_t va)
{
    partition_t* ptn = pfn_to_partition(pfn);
    ptn->owner = owner;
    ptn->va = va;
}

static uintptr_t alloc_partition_for_host_os(void)
{
    int i;
    partition_t *ptn, *tmp;

    for_each_partition_in_pool_rev(memory_pool, ptn, i)
    {
        if (ptn->owner == 0)
            continue;
        if (ptn->owner > 0) {
            tmp = find_available_partition();
            partition_migration(ptn->pfn, tmp->pfn);
        }

        update_partition_info(ptn->pfn, 0, 0);
        return ptn->pfn << PARTITION_SHIFT;
    }

    die_oom();
    return 0;
}

static int get_avail_pmp_count(enclave_context_t* ectx)
{
    int count = 0;

    if (!ectx) {
        return -1;
    }

    for (int i = 0; i < PMP_REGION_NUM; i++) {
        if (!ectx->pmp_reg->used)
            count++;
    }

    return count;
}

static region_t find_smallest_region(int eid)
{
    int i;
    partition_t* ptn;
    uintptr_t head = 0, tail = 0;
    int hit = 0;
    int min = MEMORY_POOL_PARTITION_NUM + 1;
    int len;
    region_t ret = { 0 };

    for_each_partition_in_pool(memory_pool, ptn, i)
    {
        if (ptn->owner != eid && hit) {
            len = (int)(tail - head + 1);
            if (len < min) {
                min = len;
                ret.pfn = head;
                ret.length = min;
            }
            hit = 0;
        }

        if (ptn->owner == eid && !hit) {
            head = ptn->pfn;
            tail = ptn->pfn;
            hit = 1;
            continue;
        }

        if (ptn->owner == eid && hit) {
            tail++;
        }
    }

    if (hit) {
        len = (int)(tail - head + 1);
        if (len < min) {
            min = len;
            ret.pfn = head;
            ret.length = min;
        }
    }

    return ret;
}

static region_t find_avail_region_larger_than(int length)
{
    int i;
    partition_t* ptn;
    uintptr_t head = 0, tail = 0;
    int hit = 0;
    int len;
    region_t ret = { 0 };

    for_each_partition_in_pool(memory_pool, ptn, i)
    {
        if (ptn->owner >= 0 && hit) {
            len = (int)(tail - head + 1);
            if (len > length) {
                ret.length = len;
                ret.pfn = head;
                return ret;
            }
            hit = 0;
        }

        if (ptn->owner < 0 && !hit) {
            head = ptn->pfn;
            tail = ptn->pfn;
            hit = 1;
            continue;
        }

        if (ptn->owner < 0 && hit) {
            tail++;
        }
    }

    if (hit) {
        len = (int)(tail - head + 1);
        if (len > length) {
            ret.length = len;
            ret.pfn = head;
            return ret;
        }
    }

    return ret;
}

static void memory_compaction(void)
{
    int i;
    partition_t* ptn;
    partition_t* tmp;
    int done = 1;

    for_each_partition_in_pool(memory_pool, ptn, i)
    {
        done = 1;
        if (ptn->owner < 0) {
            for (int j = 1; i + j < MEMORY_POOL_PARTITION_NUM; j++) {
                tmp = pfn_to_partition(ptn->pfn + j);
                if (tmp->owner > 0) {
                    partition_migration(tmp->pfn, ptn->pfn);
                    done = 0;
                    break;
                }
            }

            if (done) {
                return;
            }
        }
    }
}

static void* ashman_memset(void* s, int c, size_t n)
{
    char* tmp = s;

    while (n > 0) {
        --n;
        *tmp++ = c;
    }

    return s;
}

static void* ashman_memcpy(void* dst, const void* src, size_t n)
{
    char* tmp1 = dst;
    const char* tmp2 = src;

    while (n > 0) {
        --n;
        *tmp1++ = *tmp2++;
    }

    return dst;
}

static void set_partition_zero(uintptr_t pfn)
{
    char* s = (char*)(pfn << PARTITION_SHIFT);
    ashman_memset(s, 0, PARTITION_SIZE);
}

void init_memory_pool(void)
{
    int i;
    partition_t* ptn;

    for_each_partition_in_pool(memory_pool, ptn, i)
    {
        ptn->pfn = (MEMORY_POOL_START + i * PARTITION_SIZE) >> PARTITION_SHIFT;
        ptn->owner = -1;
    }
}

uintptr_t alloc_partition(enclave_context_t* ectx, uintptr_t va)
{
    int i;
    partition_t *ptn, *tmp;
    uintptr_t eid;
    uintptr_t ret = 0;
    region_t smallest, avail;
    int tried_flag = 0;

    if (!ectx) {
        return 0;
    }
    eid = ectx->id;

    // 0. (optimization) check whether there is any partition available
    if (eid == 0) { // From Host OS
        return alloc_partition_for_host_os();
    }

    // 1. Look for available partitions adjacent to allocated
    //    partitions owned by the enclave. If found, update PMP config
    //    and return the pa of the partition
    for_each_partition_in_pool(memory_pool, ptn, i)
    {
        if (ptn->owner != eid)
            continue;

        // left neighbor
        if (i >= 1) {
            tmp = pfn_to_partition(ptn->pfn - 1);
            if (tmp->owner < 0) {
                ret = tmp->pfn;
                goto found;
            }
        }

        // right neighbor
        if (i <= MEMORY_POOL_PARTITION_NUM - 1) {
            tmp = pfn_to_partition(ptn->pfn + 1);
            if (tmp->owner < 0) {
                ret = tmp->pfn;
                goto found;
            }
        }
    }

    // 2. If no such partition exists, then check whether the PMP resource
    //    has run out. If not, allocate a new partition for the enclave
    if (get_avail_pmp_count(ectx) > 0) {
        ptn = find_available_partition();
        if (!ptn) {
            die_oom();
            return 0;
        }
        for (int i = 0; i < PMP_REGION_NUM; i++) {
            if (!ectx->pmp_reg[i].used) {
                ectx->pmp_reg[i].used = 1;
                break;
            }
        }
        ret = ptn->pfn;
        goto found;
    }

    // 3. If PMP resource has run out, find the smallest contiguous memory
    //    region owned by the enclave. Assume the region is of N partitions.
    //    Look for an available memory region that consists of more than
    //    N + 1 partitions. If found, perform partition migration and allocate
    //    a partition.
try_find:

    smallest = find_smallest_region(eid);
    avail = find_avail_region_larger_than(smallest.length);

    if (avail.length) {
        for (int i = 0; i < smallest.length; i++) {
            partition_migration(smallest.pfn + i, avail.pfn + i);
        }
        ret = smallest.pfn + smallest.length;
        goto found;
    }
    if (tried_flag) {
        die_oom();
        return 0;
    }
    // 4. If still not found, do page compaction, then repeat step 3.
    memory_compaction();
    tried_flag = 1;
    goto try_find;

found:
    set_partition_zero(ret);
    update_partition_info(ret, eid, va);
    // PMP

    return ret << PARTITION_SHIFT;
}

uintptr_t free_partition(int eid)
{
    int i;
    partition_t* ptn;

    for_each_partition_in_pool(memory_pool, ptn, i)
    {
        if (ptn->owner == eid) {
            free_partition(ptn->pfn);
        }
    }
}

static void flush_tlb(void)
{
    asm volatile("sfence.vma");
}

int partition_migration(uintptr_t src_pfn, uintptr_t dst_pfn)
{
    partition_t* src_ptn = pfn_to_partition(src_pfn);
    partition_t* dst_ptn = pfn_to_partition(dst_pfn);
    uintptr_t src_pa = src_pfn << PARTITION_SHIFT;
    uintptr_t dst_pa = dst_pfn << PARTITION_SHIFT;
    uintptr_t pa_diff = dst_pa - src_pa;
    uintptr_t src_owner = src_ptn->owner;
    uintptr_t linear_start_va = src_ptn->va;
    uint32_t hartid = current_hartid();
    uintptr_t eid = (uintptr_t)enclave_on_core[hartid];
    enclave_context_t* ectx = eid_to_context(src_owner);
    char has_runtime_pt = 0;
    uintptr_t *pt_root_addr, *offset_addr;
    inverse_map_t* inv_map_addr;
    uintptr_t pt_root;
    uintptr_t va;
    uintptr_t satp;
    inverse_map_t* inv_map_entry;

    if (src_owner < 0 || ectx == NULL) {
        return 0;
    }

    if (dst_ptn->owner >= 0) {
        return 0;
    }

    pt_root_addr = (uintptr_t*)ectx->pt_root_addr;
    inv_map_addr = (inverse_map_t*)ectx->inverse_map_addr;
    offset_addr = (uintptr_t*)ectx->offset_addr;
    pt_root = *pt_root_addr;

    // 1. judge whether the section contains a base module
    if (src_pfn == PARTITION_DOWN((uintptr_t)pt_root_addr) >> PARTITION_SHIFT) {
        has_runtime_pt = 1;
    }

    // 2. Copy section content, set section VA&owner
    ashman_memcpy((void*)dst_pa, (void*)src_pa, PARTITION_SIZE);
    dst_ptn->owner = src_owner;
    dst_ptn->va = linear_start_va;

    // 3. For base module, calculate the new PA of pt_root,
    //    inv_map, and va_pa_offset.
    //    Update the enclave context, and their value accordingly
    if (has_runtime_pt) {
        ectx->pt_root_addr += pa_diff;
        ectx->inverse_map_addr += pa_diff;
        ectx->offset_addr += pa_diff;
        // **** Other context modifications here ****

        // **** END

        pt_root_addr = (uintptr_t*)ectx->pt_root_addr;
        inv_map_addr = (inverse_map_t*)ectx->inverse_map_addr;
        offset_addr = (uintptr_t*)ectx->offset_addr;

        // Update pt_root
        *pt_root_addr += pa_diff; // value of pt_root update
        pt_root = *pt_root_addr;
        satp = pt_root >> EPAGE_SHIFT;
        satp |= (uintptr_t)SATP_MODE_SV39 << SATP_MODE_SHIFT;
        if ((int)eid == src_ptn->owner) {
            csr_write(CSR_SATP, satp);
        } else {
            enclave_context_t* owner_context = eid_to_context(src_ptn->owner);
            owner_context->ns_satp = satp;
        }
        *offset_addr -= pa_diff;
    }

    // 4. Update page table
    //	a. update tree PTE
    //	b. update leaf PTE
    //		For each PA in the section:
    //		(1). updates its linear map pte
    //		(2). check the inverse map, update the PTE if PA is
    //		     in the table and update the inverse map
    if (has_runtime_pt)
        update_tree_pte(pt_root, pa_diff);

    for (uintptr_t offset = 0; offset < PARTITION_SIZE;
         offset += EPAGE_SIZE) {
        va = linear_start_va + offset;
        update_leaf_pte(pt_root, va, dst_pa + offset);
    }

    for (uintptr_t offset = 0; offset < PARTITION_SIZE;
         offset += EPAGE_SIZE) {
        inv_map_entry = look_up_inverse_map(
            (inverse_map_t*)inv_map_addr, src_pa + offset);
        if (inv_map_entry) {
            for (int i = 0; i < inv_map_entry->n; i++) {
                va = inv_map_entry->va + i * EPAGE_SIZE;
                update_leaf_pte(pt_root, va,
                    inv_map_entry->pa + pa_diff + i * EPAGE_SIZE);
            }
            inv_map_entry->pa += pa_diff;
        }
    }

    // 5. Free the source sectino
    free_partition(src_pfn);

    // 6. Flush TLB and D-cache
    flush_tlb();
    // flush_dcache_range(dst_pa, dst_pa + SECTION_SIZE);
    // invalidate_dcache_range(src_pa, src_pa + SECTION_SIZE);
    // if (!is_base_module) {
    // 	flush_dcache_range(pt_root,
    // 			   pt_root + PAGE_DIR_POOL * EPAGE_SIZE);
    // }

    return dst_pfn;
}