#include "iodev/pci/confrw.h"
#include "iodev/pci/devices.h"
#include "iodev/acpi/tables/mcfg.h"
#include "mem/paging/paging.h"
#include <stdlib.h>

size_t pci_function_cache_entries = 0;
pci_function_t* pci_function_cache = NULL;

static bool cached = false;

void pci_conf_load_cache() {
    acpi_mcfg_header_t* mcfg = (acpi_mcfg_header_t *)acpi_get_table("MCFG", 0);

    if (!mcfg)
        return;

    uint64_t table_length = mcfg->common.length - sizeof(acpi_mcfg_header_t);
    uint64_t table_entries = table_length / sizeof(acpi_mcfg_entry_t);
    
    for (uint16_t seg = 0; seg < table_entries; seg++) {
        acpi_mcfg_entry_t* segment = ACPI_MCFG_GET_ENTRY(mcfg, seg);
        for (uint8_t bus = segment->start_bus; bus < segment->end_bus; bus++) {
            uint64_t bus_offset = (uint64_t)bus << 20;
            void* bus_address = (void *)((uint64_t)segment->base + bus_offset);
            pci_dev_header_t* bus_header = (pci_dev_header_t *)paging_map_page((void *)((uintptr_t)bus_address + PAGING_VIRTUAL_OFFSET), bus_address, PAGING_FLAGS_KERNEL_PAGE);

            if (bus_header->device_id == 0x0000 || bus_header->device_id == 0xFFFF)
                continue;

            for (uint8_t device = 0; device < 32; device++) {
                uint64_t device_offset = (uint64_t)device << 15;
                void* device_address = (void *)((uint64_t)bus_address + device_offset);
                pci_dev_header_t* device_header = (pci_dev_header_t *)paging_map_page((void *)((uintptr_t)device_address + PAGING_VIRTUAL_OFFSET), device_address, PAGING_FLAGS_KERNEL_PAGE);

                if (device_header->device_id == 0x0000 || device_header->device_id == 0xFFFF)
                    continue;

                for (uint8_t function = 0; function < 8; function++) {
                    uint64_t function_offset = (uint64_t)function << 12;
                    void* function_address = (void *)((uint64_t)device_address + function_offset);
                    pci_dev_header_t* function_header = (pci_dev_header_t *)paging_map_page((void *)((uintptr_t)function_address + PAGING_VIRTUAL_OFFSET), function_address, PAGING_FLAGS_KERNEL_PAGE);

                    if (function_header->device_id == 0x0000 || function_header->device_id == 0xFFFF)
                        continue;

                    pci_function_cache_entries++;

                    if (!pci_function_cache)
                        pci_function_cache = (pci_function_t *)malloc(sizeof(pci_function_t) * pci_function_cache_entries);
                    else
                        pci_function_cache = (pci_function_t *)realloc(pci_function_cache, sizeof(pci_function_t) * pci_function_cache_entries);
                    
                    pci_function_t* cached_function = &pci_function_cache[pci_function_cache_entries - 1];

                    cached_function->base = (void *)function_header;
                    cached_function->function = function;
                    cached_function->device = device;
                    cached_function->bus = bus;
                    cached_function->segment = seg;
                }
            }
        }
    }
    
    cached = true;

    for (size_t i = 0; i < pci_function_cache_entries; i++) {
        pci_function_t* function = &pci_function_cache[i];
        pci_dev_header_t* header = (pci_dev_header_t *)function->base;
    }
}

void pci_conf_write_byte(uint16_t segment, uint8_t bus, uint8_t device, uint8_t function, uint16_t offset, uint8_t value) {
    if (!cached)
        pci_conf_load_cache();
    
    for (size_t i = 0; i < pci_function_cache_entries; i++) {
        pci_function_t* iterator = &pci_function_cache[i];
        
        if (iterator->segment != segment) continue;

        if (iterator->bus != bus) continue;

        if (iterator->device != device) continue;

        if (iterator->function != function) continue;

        uint8_t volatile* destination = (uint8_t *)((uintptr_t)iterator->base + offset);
        *destination = value;
    }
}

void pci_conf_write_word(uint16_t segment, uint8_t bus, uint8_t device, uint8_t function, uint16_t offset, uint16_t value) {
    if (!cached)
        pci_conf_load_cache();
    
    for (size_t i = 0; i < pci_function_cache_entries; i++) {
        pci_function_t* iterator = &pci_function_cache[i];
        
        if (iterator->segment != segment) continue;

        if (iterator->bus != bus) continue;

        if (iterator->device != device) continue;

        if (iterator->function != function) continue;

        uint16_t volatile* destination = (uint16_t *)((uintptr_t)iterator->base + offset);
        *destination = value;
    }
}

void pci_conf_write_long(uint16_t segment, uint8_t bus, uint8_t device, uint8_t function, uint16_t offset, uint32_t value) {
    if (!cached)
        pci_conf_load_cache();
    
    for (size_t i = 0; i < pci_function_cache_entries; i++) {
        pci_function_t* iterator = &pci_function_cache[i];
        
        if (iterator->segment != segment) continue;

        if (iterator->bus != bus) continue;

        if (iterator->device != device) continue;

        if (iterator->function != function) continue;

        uint32_t volatile* destination = (uint32_t *)((uintptr_t)iterator->base + offset);
        *destination = value;
    }
}

uint8_t pci_conf_read_byte(uint16_t segment, uint8_t bus, uint8_t device, uint8_t function, uint16_t offset) {
    if (!cached)
        pci_conf_load_cache();
    
    for (size_t i = 0; i < pci_function_cache_entries; i++) {
        pci_function_t* iterator = &pci_function_cache[i];
        
        if (iterator->segment != segment) continue;

        if (iterator->bus != bus) continue;

        if (iterator->device != device) continue;

        if (iterator->function != function) continue;

        uint8_t volatile* destination = (uint8_t *)((uintptr_t)iterator->base + offset);
        uint8_t value = *destination;
        return value;
    }
    return 0;
}

uint16_t pci_conf_read_word(uint16_t segment, uint8_t bus, uint8_t device, uint8_t function, uint16_t offset) {
    if (!cached)
        pci_conf_load_cache();
    
    for (size_t i = 0; i < pci_function_cache_entries; i++) {
        pci_function_t* iterator = &pci_function_cache[i];
        
        if (iterator->segment != segment) continue;

        if (iterator->bus != bus) continue;

        if (iterator->device != device) continue;

        if (iterator->function != function) continue;

        uint16_t volatile* destination = (uint16_t *)((uintptr_t)iterator->base + offset);
        uint16_t value = *destination;
        return value;
    }
    return 0;
}

uint32_t pci_conf_read_long(uint16_t segment, uint8_t bus, uint8_t device, uint8_t function, uint16_t offset) {
    if (!cached)
        pci_conf_load_cache();
    
    for (size_t i = 0; i < pci_function_cache_entries; i++) {
        pci_function_t* iterator = &pci_function_cache[i];
        
        if (iterator->segment != segment) continue;

        if (iterator->bus != bus) continue;

        if (iterator->device != device) continue;

        if (iterator->function != function) continue;

        uint32_t volatile* destination = (uint32_t *)((uintptr_t)iterator->base + offset);
        uint32_t value = *destination;
        return value;
    }
    return 0;
}
