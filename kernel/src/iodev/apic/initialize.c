#include "iodev/acpi/tables/madt.h"
#include "iodev/8259/pic.h"
#include "cpu/interrupts/idt.h"
#include "cpu/tss/tss.h"
#include "iodev/apic/lapic.h"
#include "iodev/apic/ioapic.h"
#include <cpuid.h>
#include <stdbool.h>

static void detect_apic() {
    uint32_t erax, erbx, ercx, erdx;
    __cpuid(1, erax, erbx, ercx, erdx);

    bool apic = (erdx >> 9) & 1;

    if (!apic) {
        __asm__ volatile ("cli; hlt");
    }
}

// TODO: make this dynamic
static uint8_t gsi_map[24];

uint8_t apic_io_get_gsi(uint8_t irq) {
    return gsi_map[irq];
}

static void configure_local_apic(acpi_madt_header_t* madt) {
    apic_local_set_base((void *)(uintptr_t)madt->lapic_address);

    apic_local_write(APIC_LOCAL_REGISTER_SPURIOUS_INT_VECTOR, 0x100 | apic_local_read(APIC_LOCAL_REGISTER_SPURIOUS_INT_VECTOR));

    uint32_t lapic_id = apic_local_read(APIC_LOCAL_REGISTER_ID);

    const uint8_t nmi = 0x04 & 0x07;

    for (acpi_madt_record_t* madt_record = (acpi_madt_record_t *)((uintptr_t)madt + sizeof(acpi_madt_header_t));;) {
        if (((uintptr_t)madt_record - (uintptr_t)madt) >= madt->common.length)
            break;

        madt_record = (acpi_madt_record_t *)((uintptr_t)madt_record + madt_record->length);

        if (madt_record->type == ACPI_MADT_RECORD_TYPE_LOCAL_NMI) {
            acpi_madt_record_nmi_t* lint01_record = (acpi_madt_record_nmi_t *)madt_record;
            if (lint01_record->processor_id != lapic_id && lint01_record->processor_id != 0xff)
                continue;
            
            apic_local_register_t lvt = lint01_record->lint_no == 0 ? APIC_LOCAL_REGISTER_LVT_LINT0 : APIC_LOCAL_REGISTER_LVT_LINT1;

            uint32_t entry = 0x00000000;
            entry |= (nmi & 0x07) << 8;

            bool level_triggered = !!(lint01_record->flags & ACPI_MADT_RECORD_ISO_NMI_FLAG_LEVEL_TRIGGERED);
            bool active_low = !!(lint01_record->flags & ACPI_MADT_RECORD_ISO_NMI_FLAG_ACTIVE_LOW);

            entry |= (uint32_t)level_triggered << 13;
            entry |= (uint32_t)active_low << 15;

            apic_local_write(lvt, entry);
        }
    }
}

static void initialize_gsi_map() {
    for (uint8_t i = 0; i < 24; i++)
        gsi_map[i] = i;
}

static void configure_io_apic(acpi_madt_header_t* madt) {
    for (acpi_madt_record_t* madt_record = (acpi_madt_record_t *)((uintptr_t)madt + sizeof(acpi_madt_header_t));;) {
        if (((uintptr_t)madt_record - (uintptr_t)madt) >= madt->common.length)
            break;

        madt_record = (acpi_madt_record_t *)((uintptr_t)madt_record + madt_record->length);

        if (madt_record->type == ACPI_MADT_RECORD_TYPE_IOAPIC) {
            acpi_madt_record_ioapic_t* ioapic = (acpi_madt_record_ioapic_t *)madt_record;
            apic_io_register_controller(*ioapic);
        }
    }

    initialize_gsi_map();

    for (acpi_madt_record_t* madt_record = (acpi_madt_record_t *)((uintptr_t)madt + sizeof(acpi_madt_header_t));;) {
        if (((uintptr_t)madt_record - (uintptr_t)madt) >= madt->common.length)
            break;

        madt_record = (acpi_madt_record_t *)((uintptr_t)madt_record + madt_record->length);

        if (madt_record->type == ACPI_MADT_RECORD_TYPE_ISO) {
            acpi_madt_record_iso_t* override = (acpi_madt_record_iso_t *)madt_record;
            apic_io_redirect_irq(override->gsi, 
                                0x00, 
                                !!(override->flags & ACPI_MADT_RECORD_ISO_NMI_FLAG_ACTIVE_LOW), 
                                !!(override->flags & ACPI_MADT_RECORD_ISO_NMI_FLAG_LEVEL_TRIGGERED));

            gsi_map[override->irq_source] = override->gsi;
        }
    }
}

void apic_initialize(void);
void apic_initialize() {
    acpi_madt_header_t* madt = ACPI_MADT_GET();

    pic_disable();

    detect_apic();

    configure_local_apic(madt);

    configure_io_apic(madt);
}
