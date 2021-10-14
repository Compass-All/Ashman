#include <enclave.h>

enclave_context_t* eid_to_context(uintptr_t eid)
{
    return &enclaves[eid];
}

int map_register(enclave_context_t* ectx, uintptr_t pt_root_addr,
    uintptr_t inverse_map_addr, uintptr_t offset_addr)
{
    if (!(pt_root_addr && inverse_map_addr && offset_addr)) {
        return 1;
    }
    ectx->pt_root_addr = pt_root_addr;
    ectx->inverse_map_addr = inverse_map_addr;
    ectx->offset_addr = offset_addr;
    return 0;
}