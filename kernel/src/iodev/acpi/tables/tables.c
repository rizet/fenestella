#include "iodev/acpi/tables/tables.h"
#include "mem/paging/paging.h"
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <string.h>

extern acpi_rdsp2_t* acpi_rsdp;
acpi_rdsp2_t* acpi_rsdp = NULL;

void acpi_load_rsdp(void* rsdp) {
    acpi_rsdp = (acpi_rdsp2_t *)rsdp;
}

acpi_sdt_header_t* acpi_rsdt_get(uint16_t index);
acpi_sdt_header_t* acpi_rsdt_get(uint16_t index) {
    acpi_sdt_header_t* rsdt_header = (acpi_sdt_header_t *)((uint64_t)(acpi_rsdp->rsdt) + PAGING_VIRTUAL_OFFSET);

    uint64_t rsdt_entries = (rsdt_header->length - sizeof(acpi_sdt_header_t)) / sizeof(uintptr_t);

    if (index >= rsdt_entries)
        return (acpi_sdt_header_t *)NULL;

    uint32_t* rsdt = (uint32_t *)((uint64_t)acpi_rsdp->rsdt + sizeof(acpi_sdt_header_t) + PAGING_VIRTUAL_OFFSET);

    return (acpi_sdt_header_t *)((uint64_t)rsdt[index] + PAGING_VIRTUAL_OFFSET);
}

acpi_sdt_header_t* acpi_xsdt_get(uint16_t index);
acpi_sdt_header_t* acpi_xsdt_get(uint16_t index) {
    acpi_sdt_header_t* xsdt_header = (acpi_sdt_header_t *)((uint64_t)(acpi_rsdp->xsdt) + PAGING_VIRTUAL_OFFSET);

    uint64_t xsdt_entries = (xsdt_header->length - sizeof(acpi_sdt_header_t)) / sizeof(uintptr_t);

    if (index >= xsdt_entries)
        return (acpi_sdt_header_t *)NULL;

    uint64_t* xsdt = (uint64_t *)((uint64_t)acpi_rsdp->xsdt + sizeof(acpi_sdt_header_t) + PAGING_VIRTUAL_OFFSET);

    return (acpi_sdt_header_t *)((uint64_t)xsdt[index] + PAGING_VIRTUAL_OFFSET);
}

acpi_sdt_header_t* acpi_get_table(char* signature, uint16_t index) {
    if (acpi_rsdp nullvptr)
        return (acpi_sdt_header_t *)NULL;

    if (strlen(signature) != 4)
        return (acpi_sdt_header_t *)NULL;

    bool xsdt = (!!acpi_rsdp->xsdt);

    uint16_t table_occurence = 0;

    for (uint16_t i = 0; ; i++) {
        acpi_sdt_header_t* header;

        if (xsdt)
            header = acpi_xsdt_get(i);
        else 
            header = acpi_rsdt_get(i);

        if (!header)
            break;

        if (!strncmp((const char *)&header->signature[0], signature, 4) && table_occurence == index)
            return header;
        else if (!strncmp((const char *)&header->signature[0], signature, 4))
            table_occurence++;
    }

    return (acpi_sdt_header_t *)NULL;
}
