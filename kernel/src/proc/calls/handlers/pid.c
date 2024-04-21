#include "iodev/io.h"
#include "cpu/tss/tss.h"
#include "proc/task/task.h"
#include <stdint.h>

uint64_t pid() {
    return ((gs_kernel_base_t *)rdmsr(IA32_GS_BASE))->pid;
}