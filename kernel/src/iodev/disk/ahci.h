#pragma once
#include "iodev/pci/devices.h"
#include <stdint.h>
#include <stdbool.h>

typedef struct {
    uint32_t    command_list_base_lower;
    uint32_t    command_list_base_upper;
    uint32_t    fis_base_lower;
    uint32_t    fis_base_upper;
    uint32_t    interrupt_status;
    uint32_t    interrupt_enable;
    uint32_t    command_status;
    uint32_t    reserved;
    uint32_t    task_file_data;
    uint32_t    signature;
    uint32_t    sata_status;
    uint32_t    sata_control;
    uint32_t    sata_error;
    uint32_t    sata_active;
    uint32_t    command_issue;
    uint32_t    sata_notification;
    uint32_t    fis_based_switch_control;
    uint32_t    reserved_block[0x0B];
    uint32_t    vendor_specific[0x04];
} ahci_hba_port_t;

typedef struct {
    uint32_t        host_capabilities;
    uint32_t        global_host_control;
    uint32_t        interrupt_status;
    uint32_t        ports_implemented;
    uint32_t        version;
    uint32_t        ccc_control;
    uint32_t        ccc_ports;
    uint32_t        enclosure_management_location;
    uint32_t        enclosure_management_control;
    uint32_t        host_capabilities_extended;
    uint32_t        bios_handoff_control_status;
    uint8_t         reserved_block[0x74];
    uint8_t         vendor_specific[0x60];
    ahci_hba_port_t ports[1];
} ahci_hba_mem_t;

typedef struct {
    ahci_hba_mem_t*     hba_mem_abar;
    pci_dev_header_t*   pci_device;
} ahci_bus_handle_t;

typedef struct s_ahci_drive_handle {
    struct s_ahci_drive_handle* next;

} ahci_drive_handle_t;

// Returns null if no drives are found
const ahci_drive_handle_t* ahci_get_first_drive();

// Returns false if driver is not available, or there is no AHCI controller
const bool ahci_driver_available();
