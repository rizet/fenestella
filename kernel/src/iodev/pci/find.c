#include <string.h>
#include "iodev/uart/uartsh.h"
#include "iodev/pci/devices.h"
#include <stdio.h>

pci_function_data_t* find_device(uint8_t class, uint8_t subclass, uint8_t interface) {
    for (size_t i = 0; i < pci_function_cache_entries; i++) {
        pci_dev_header_t* device = (pci_dev_header_t *)pci_function_cache[i].base;
        if (device->device_class != class) continue;
        if (device->device_subclass != subclass) continue;
        if (device->program_interface != interface) continue;

        return &pci_function_cache[i];
    }
    return NULL;
}

void __uartsh_pci_dump() {
    printf("\r\nPCI devices:\r\n\r\n");
    for (size_t i = 0; i < pci_function_cache_entries; i++) {
        pci_function_data_t* function = &pci_function_cache[i];
        pci_dev_header_t* header = function->base;
        printf("\t%x:%x:%x:%x, id: %x:%x, class: %x:%x:%x\r\n", function->segment, function->bus, function->device, function->function, header->vendor_id, header->device_id, header->device_class, header->device_subclass, header->program_interface);
    }
}
