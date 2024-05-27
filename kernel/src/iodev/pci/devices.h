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
	pci_dev_header_t    base;
	uint32_t            bar0;
	uint32_t            bar1;
	uint32_t            bar2;
	uint32_t            bar3;
	uint32_t            bar4;
	uint32_t            bar5;
	uint32_t            cardbus_cis_pointer;
	uint16_t            subsystem_vendor_id;
	uint16_t            subsystem_id;
	uint32_t            expansion_rom_base_address;
	uint8_t             capabilities_pointer;
	uint8_t             reserved_block[7];
	uint8_t             interrupt_line;
	uint8_t             interrupt_pin;
	uint8_t             min_grant;
	uint8_t             max_latency;
} pci_ext_header_0_t;

typedef struct {
    pci_dev_header_t*	base;
    uint16_t			segment;
    uint8_t				bus;
    uint8_t				device;
    uint8_t				function;
    uint32_t : 24;
} pci_function_data_t;

extern size_t pci_function_cache_entries;
extern pci_function_data_t* pci_function_cache;

// Returns null if none found
pci_function_data_t* find_device(uint8_t class, uint8_t subclass, uint8_t interface);
