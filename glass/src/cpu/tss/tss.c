#include "cpu/tss/tss.h"
#include "cpu/gdt/gdt.h"
#include "mem/paging/paging.h"
#include "mem/pmm/pmm.h"
#include "extra/conv.h"
#include "iodev/uart/uartsh.h"
#include <string.h>
#include <stdio.h>

extern tss_t tss_descriptors[];
tss_t tss_descriptors[TSS_MAX_CPUS];

static uint8_t ist_index = 0;

uint8_t tss_add_stack(int num_cpu) {
    if (ist_index >= 7)
        return 1;

    void* stack = pmm_alloc_pool(IST_STACK_PAGES);

    tss_descriptors[num_cpu].ist[ist_index] = (uint64_t)stack + (PAGING_PAGE_SIZE * IST_STACK_PAGES);
    ist_index++;
    return ist_index;
}

uint8_t tss_get_num_stacks(int num_cpu) {
    return ist_index - 1;
}

void tss_install(int num_cpu) {
    uint64_t tss_base = (uint64_t)&tss_descriptors[num_cpu];
    memset((void *)tss_base, 0, sizeof(tss_t));
    
    tss_add_stack(num_cpu);
    tss_add_stack(num_cpu);
    tss_add_stack(num_cpu);

    tss_descriptors[num_cpu].rsp[0] = tss_descriptors[num_cpu].ist[0];

    tss_descriptors[num_cpu].io_map = sizeof(tss_t);

    tss_reload(gdt_install_tss(tss_base));
}

tss_t* tss_get(int num_cpu) {
    return &tss_descriptors[num_cpu];
}

void* tss_get_stack(int num_cpu, uint8_t stack) {
    return (void*)tss_descriptors[num_cpu].ist[stack];
}

void __uartsh_tss_dump() {
    printf("\r\nTSS dump:\r\n");
    for (int cpu = 0; cpu < TSS_MAX_CPUS; ++cpu) {
        tss_t* tss = tss_get(cpu);
        printf("\tTSS for CPU %d\r\n", cpu);
        printf("\t\tRSP0: %x\r\n", tss->rsp[0]);
        printf("\t\tRSP1: %x\r\n", tss->rsp[1]);
        printf("\t\tRSP2: %x\r\n", tss->rsp[2]);
        printf("\t\tIST1: %x\r\n", tss->ist[0]);
        printf("\t\tIST2: %x\r\n", tss->ist[1]);
        printf("\t\tIST3: %x\r\n", tss->ist[2]);
        printf("\t\tIST4: %x\r\n", tss->ist[3]);
        printf("\t\tIST5: %x\r\n", tss->ist[4]);
        printf("\t\tIST6: %x\r\n", tss->ist[5]);
        printf("\t\tIST7: %x\r\n", tss->ist[6]);
        printf("\t\tIO Map: %x\r\n", tss->io_map);
        printf("\r\n");
    }
}
