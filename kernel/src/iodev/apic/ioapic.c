#include "iodev/apic/ioapic.h"
#include "mem/paging/paging.h"
#include <stdlib.h>

apic_io_controller_node_t* ioapics = NULL;

void apic_io_register_controller(acpi_madt_record_ioapic_t controller) {
    apic_io_controller_node_t* new_ioapic;

    if (!ioapics) {
        ioapics = (apic_io_controller_node_t *)malloc(sizeof(apic_io_controller_node_t));
        new_ioapic = ioapics;
        new_ioapic->next = NULL;
    }
    else {
        apic_io_controller_node_t* before_new;
        for (before_new = ioapics; !!before_new->next; before_new = before_new->next);
        before_new->next = (apic_io_controller_node_t *)malloc(sizeof(apic_io_controller_node_t));
        new_ioapic = before_new->next;
    }

    new_ioapic->base = (void *)((uintptr_t)controller.ioapic_address + PAGING_VIRTUAL_OFFSET);
    new_ioapic->ioapic_id = controller.ioapic_id;

    paging_map_page((void *)((uintptr_t)controller.ioapic_address + PAGING_VIRTUAL_OFFSET), (void *)(uintptr_t)controller.ioapic_address, PAGING_FLAGS_KERNEL_PAGE);
}

void* apic_io_get_base(uint64_t ioapic_id) {
    apic_io_controller_node_t* ioapic;

    for (ioapic = ioapics; !!ioapic; ioapic = ioapic->next)
        if (ioapic->ioapic_id == ioapic_id)
            return (void *)(ioapic->base);

    return NULL;
}

void apic_io_mask_irq(uint8_t irq) {
    apic_io_write(0, IOAPIC_REGISTER_REDIRECTION(irq), apic_io_read(0, IOAPIC_REGISTER_REDIRECTION(irq)) | 0x10000);
}

void apic_io_unmask_irq(uint8_t irq) {
    apic_io_write(0, IOAPIC_REGISTER_REDIRECTION(irq), apic_io_read(0, IOAPIC_REGISTER_REDIRECTION(irq)) & ~0x10000);
}

void apic_io_redirect_irq(uint8_t irq, uint8_t vector, bool alow, bool ltriggered) {
    uint32_t destination = 0x00000000;
    uint32_t redirect = 0x00000000;

    redirect |= vector;
    redirect |= ((uint32_t)alow << 13);
    redirect |= ((uint32_t)ltriggered << 15);
    redirect |= (1 << 16);

    apic_io_write(0, IOAPIC_REGISTER_REDIRECTION(irq) + 1, destination);
    apic_io_write(0, IOAPIC_REGISTER_REDIRECTION(irq), redirect);
}

apic_io_redirect_t apic_io_get_redirect(uint8_t irq) {
    uint32_t entry = apic_io_read(0, IOAPIC_REGISTER_REDIRECTION(irq));

    apic_io_redirect_t redirect;

    redirect.irq = irq;
    redirect.vector = entry & 0xFF;

    redirect.active_low = entry & (1 << 13);
    redirect.level_triggered = entry & (1 << 15);
    redirect.masked = entry & (1 << 16);

    return redirect;
}
