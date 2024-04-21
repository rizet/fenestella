#pragma once
#include "iodev/acpi/tables/tables.h"

#define ACPI_MADT_RECORD_TYPE_LAPIC                     0
#define ACPI_MADT_RECORD_TYPE_IOAPIC                    1
#define ACPI_MADT_RECORD_TYPE_ISO                       2
#define ACPI_MADT_RECORD_TYPE_IOAPIC_NMI                3
#define ACPI_MADT_RECORD_TYPE_LOCAL_NMI                 4
#define ACPI_MADT_RECORD_TYPE_LAPIC_ADDRESS_OVERRIDE    5

#define ACPI_MADT_RECORD_ISO_NMI_FLAG_ACTIVE_LOW        0x02
#define ACPI_MADT_RECORD_ISO_NMI_FLAG_LEVEL_TRIGGERED   0x80

#define ACPI_MADT_GET() (acpi_madt_header_t *)acpi_get_table("APIC", 0)

typedef struct {
    acpi_sdt_header_t   common;
    uint32_t            lapic_address;
    uint32_t            flags;
} __attribute__((packed)) acpi_madt_header_t;

typedef struct {
    uint8_t type;
    uint8_t length;
} acpi_madt_record_t;

typedef struct {
    acpi_madt_record_t  common;
    uint8_t             processor_id;
    uint8_t             apic_id;
    uint32_t            flags;
} __attribute__((packed)) acpi_madt_record_lapic_t;

typedef struct {
    acpi_madt_record_t  common;
    uint8_t             ioapic_id;
    uint8_t             rsv0;
    uint32_t            ioapic_address;
    uint32_t            gsi_base;
} __attribute__((packed)) acpi_madt_record_ioapic_t;

typedef struct {
    acpi_madt_record_t  common;
    uint8_t             bus_source;
    uint8_t             irq_source;
    uint32_t            gsi;
    uint16_t            flags;
} __attribute__((packed)) acpi_madt_record_iso_t;

typedef struct {
    acpi_madt_record_t  common;
    uint8_t             processor_id;
    uint16_t            flags;
    uint8_t             lint_no;
} __attribute__((packed)) acpi_madt_record_nmi_t;

typedef struct {
    acpi_madt_record_t  common;
    uint16_t            rsv0;
    uint64_t            lapic_base;
} __attribute__((packed)) acpi_madt_record_lapic_base_override_t;
