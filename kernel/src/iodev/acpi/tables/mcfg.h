#pragma once
#include "iodev/acpi/tables/tables.h"

#define ACPI_MCFG_GET() (acpi_mcfg_header_t *)acpi_get_table("MCFG", 0)
#define ACPI_MCFG_GET_ENTRY(mcfg, index) (acpi_mcfg_entry_t *)(((uintptr_t)mcfg + sizeof(acpi_mcfg_header_t)) + (sizeof(acpi_mcfg_entry_t) * index))

typedef struct {
    acpi_sdt_header_t   common;
    uint64_t            rsv0;
} __attribute__((packed)) acpi_mcfg_header_t;

typedef struct {
    void*               base;
    uint16_t            segment;
    uint8_t             start_bus;
    uint8_t             end_bus;
    uint32_t            rsv0;
} __attribute__((packed)) acpi_mcfg_entry_t;
