#pragma once
#include <stdint.h>

typedef struct {
    uint8_t     signature[4];
    uint32_t    length;
    uint8_t     revision;
    uint8_t     checksum;
    uint8_t     vendor_id[6];
    uint8_t     vendor_table_id[8];
    uint32_t    vendor_revision;
    uint32_t    creator_id;
    uint32_t    creator_revision;
} __attribute__((packed)) acpi_sdt_header_t;

typedef struct {
    uint8_t             signature[8];
    uint8_t             checksum;
    uint8_t             vendor_id[6];
    uint8_t             revision;
    uint32_t            rsdt;
    uint32_t            length;
    acpi_sdt_header_t*  xsdt;
    uint8_t             xchecksum;
    uint8_t             rsv0[3];
} __attribute__((packed)) acpi_rdsp2_t;

void                acpi_load_rsdp(void* rsdp);
acpi_sdt_header_t*  acpi_get_table(char* signature, uint16_t index);
