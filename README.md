# Ashman: A novel memory management for RISC-V enclaves

## Usage

1. Modify configurations in header files if necessary.
1. Copy the header/source files into corresponding directories into the firmware code (OpenSBI suggested).
1. Adjust your runtime and firmware code to utilize our functions.
1. Build and test your firmware.

You may want to edit the following configurations:

- Physical addresses of enclave memory pool

```c
// mempool.h

#define MEMORY_POOL_START 0x140000000
#define MEMORY_POOL_END 0x170000000
```

- The number of inverse map entries

```c
// epage.h

#define INVERSE_MAP_ENTRY_NUM 1024
```

- The number of allowed PMP entries

```c
// pmp.h

#define PMP_REGION_NUM 8
```

- The number of allowed enclaves and harts for enclaves

```c
// enclave.h

#define ENCLAVE_NUM 180
#define HART_NUM 4
```

- Enclave context definition

```c
// enclave.h

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
```

## Citation

If you want to cite the project, please use the following bibtex:

```
@inproceedings{haonan2021ashman
    title={A Novel Memory Management for {RISC-V} Enclaves},
    author={Haonan Li and Weijie Huang and Mingde Ren and Hongyi Lu and Zhenyu Ning and Heming Cui and Fengwei Zhang},
    year={2021},
    booktitle = {Proceedings of the Hardware and Architectural Support for Security and Privacy},
    series = {HASP’21}
}
```
