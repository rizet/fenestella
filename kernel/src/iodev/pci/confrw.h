#pragma once
#include <stdint.h>

void        pci_conf_load_cache(void);

void        pci_conf_write_byte(uint16_t segment, uint8_t bus, uint8_t device, uint8_t function, uint16_t offset, uint8_t value);
void        pci_conf_write_word(uint16_t segment, uint8_t bus, uint8_t device, uint8_t function, uint16_t offset, uint16_t value);
void        pci_conf_write_long(uint16_t segment, uint8_t bus, uint8_t device, uint8_t function, uint16_t offset, uint32_t value);

uint8_t     pci_conf_read_byte(uint16_t segment, uint8_t bus, uint8_t device, uint8_t function, uint16_t offset);
uint16_t    pci_conf_read_word(uint16_t segment, uint8_t bus, uint8_t device, uint8_t function, uint16_t offset);
uint32_t    pci_conf_read_long(uint16_t segment, uint8_t bus, uint8_t device, uint8_t function, uint16_t offset);
