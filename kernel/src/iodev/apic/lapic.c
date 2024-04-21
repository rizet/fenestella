#include "iodev/apic/lapic.h"
#include <stddef.h>

uint32_t* apic_virtual_base = NULL;

void apic_local_send_eoi() {
    if (apic_virtual_base nullvptr)
        return;

    apic_local_write(APIC_LOCAL_REGISTER_EOI, 0x00000000);
}

void apic_local_send_ipi(uint8_t lapic_id, uint8_t vector) {
    uint32_t ipi = 0x00000000;

    apic_local_write(APIC_LOCAL_REGISTER_INTERRUPT_DESTINATION, (uint32_t)lapic_id << 24);

    ipi |= (uint32_t)vector;
    ipi |= (uint32_t)(0x00 << 8);
    ipi |= (uint32_t)(0x00 << 11);
    ipi |= (uint32_t)(0x01 << 14);
    ipi |= (uint32_t)(0x00 << 15);
    ipi |= (uint32_t)(0x00 << 18);

    apic_local_write(APIC_LOCAL_REGISTER_INTERRUPT_COMMAND, ipi);

    while (apic_local_read(APIC_LOCAL_REGISTER_INTERRUPT_COMMAND) & 0x1000);
}

void apic_local_cast_ipi(uint8_t vector) {
    uint32_t ipi = 0x00000000;

    ipi |= (uint32_t)vector;
    ipi |= (uint32_t)(0x00 << 8);
    ipi |= (uint32_t)(0x00 << 11);
    ipi |= (uint32_t)(0x01 << 14);
    ipi |= (uint32_t)(0x00 << 15);
    ipi |= (uint32_t)(0x03 << 18);

    apic_local_write(APIC_LOCAL_REGISTER_INTERRUPT_COMMAND, ipi);

    while (apic_local_read(APIC_LOCAL_REGISTER_INTERRUPT_COMMAND) & 0x1000);
}
