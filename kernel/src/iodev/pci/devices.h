#pragma once
#include <stdint.h>
#include <stddef.h>

typedef struct {
    uint16_t    vendor_id;
    uint16_t    device_id;
    uint16_t    command;
    uint16_t    status;
    uint8_t     revision_id;
    uint8_t     program_interface;
    uint8_t     device_subclass;
    uint8_t     device_class;
    uint8_t     cache_line_size;
    uint8_t     latency_timer;
    uint8_t     header_type;
    uint8_t     bist;
} pci_dev_header_t;

typedef struct {
    void* base;
    uint16_t segment;
    uint8_t bus;
    uint8_t device;
    uint8_t function;
    uint32_t : 24;
} pci_function_t;

extern size_t pci_function_cache_entries;
extern pci_function_t* pci_function_cache;

pci_function_t* find_device(uint8_t class, uint8_t subclass, uint8_t interface);
