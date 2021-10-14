#ifndef ASHMAN_ENCLAVE_H
#define ASHMAN_ENCLAVE_H

#define ENCLAVE_NUM 180
#define HART_NUM 4

#ifndef __ASSEMBLER__

#include <pmp.h>

typedef struct __enc_ctx {
    uintptr_t id;

    // **** Other context info here ****

    // **** END
    uintptr_t ns_satp;

    uintptr_t pt_root_addr;
    uintptr_t inverse_map_addr;
    uintptr_t offset_addr;
    pmp_region_t pmp_reg[PMP_REGION_NUM];
} enclave_context_t;

extern enclave_context_t enclaves[ENCLAVE_NUM + 1];
extern int enclave_on_core[HART_NUM];

#ifndef csr_read
#define csr_read(csr)                         \
    ({                                        \
        register unsigned long __v;           \
        __asm__ __volatile__("csrr %0, " #csr \
                             : "=r"(__v)      \
                             :                \
                             : "memory");     \
        __v;                                  \
    })
#endif // !csr_read

#ifndef csr_write
#define csr_write(csr, val)                       \
    ({                                            \
        unsigned long __v = (unsigned long)(val); \
        __asm__ __volatile__("csrw " #csr ", %0"  \
                             :                    \
                             : "rK"(__v)          \
                             : "memory");         \
    })
#endif // !csr_write

#ifndef CSR_MHARTID
#define CSR_MHARTID 0xf14
#endif // !CSR_MHARTOD

#ifndef CSR_SATP
#define CSR_SATP 0x180
#endif // !CSR_SATP

#define current_hartid() ((unsigned int)csr_read(CSR_MHARTID))

enclave_context_t* eid_to_context(uintptr_t eid);

int map_register(enclave_context_t* ectx, uintptr_t pt_root_addr,
    uintptr_t inverse_map_addr, uintptr_t offset_addr);

#endif // !__ASSEMBLER__

#endif // !ASHMAN_ENCLAVE_H