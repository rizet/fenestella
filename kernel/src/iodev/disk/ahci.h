#pragma once
#include "iodev/pci/devices.h"
#include <stdint.h>
#include <stdbool.h>

typedef enum {
    AHCI_CONTROLLER_PORT_TYPE_NONE,
    AHCI_CONTROLLER_PORT_TYPE_SATA,
    AHCI_CONTROLLER_PORT_TYPE_SATAPI,
    AHCI_CONTROLLER_PORT_TYPE_SEMB,
    AHCI_CONTROLLER_PORT_TYPE_PM,
} ahci_controller_port_type_t;

typedef enum {
    AHCI_FIS_TYPE_REG_H2D   = 0x27,
    AHCI_FIS_TYPE_REG_D2H   = 0x34,
    AHCI_FIS_TYPE_DMA_ACT   = 0x39,
    AHCI_FIS_TYPE_DMA_SETUP = 0x41,
    AHCI_FIS_TYPE_DATA      = 0x46,
    AHCI_FIS_TYPE_BIST      = 0x58,
    AHCI_FIS_TYPE_PIO_SETUP = 0x5F,
    AHCI_FIS_TYPE_DEV_BITS  = 0xA1,
} ahci_controller_fis_type_t;

typedef volatile struct {
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
} __attribute__((__packed__)) ahci_hba_port_t;

typedef volatile struct {
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
    ahci_hba_port_t ports[0];
} __attribute__((__packed__)) ahci_hba_mem_t;

typedef struct ahci_hba_prdt_entry_t {
    uint32_t    data_base_lower;
    uint32_t    data_base_upper;
    uint32_t    reserved;

    uint32_t    byte_count          :   22;
    uint32_t    reserved_bits       :   9;
    uint32_t    interrupt_please    :   1;
} __attribute__((__packed__)) ahci_hba_prdt_entry_t;

typedef struct {
    uint8_t                 command_fis[0x40];
    uint8_t                 atapi_command[0x10];
    uint8_t                 reserved[0x30];
    ahci_hba_prdt_entry_t   prdt_entries[0];
} __attribute__((__packed__)) ahci_hba_command_table_t;

typedef struct {
    uint8_t     command_fis_length  :   5;
    uint8_t     atapi_field         :   1;
    uint8_t     write_field         :   1;
    uint8_t     prefetchable        :   1;

    uint8_t     reset_field         :   1;
    uint8_t     bist_field          :   1;
    uint8_t     clear_busy          :   1;
    uint8_t     reserved_bits       :   1;
    uint8_t     port_multiplier     :   1;
    
    uint16_t    prdt_length;
    uint32_t    prdb_count;
    uint32_t    command_table_base_lower;
    uint32_t    command_table_base_upper;
    uint32_t    reserved_block[4];
} __attribute__((__packed__)) ahci_hba_command_header_t;

typedef struct {
    uint8_t     fis_type;

    uint8_t     port_multiplier :   4;
    uint8_t     reserved        :   3;
    uint8_t     command_control :   1;

    uint8_t     command;
    uint8_t     feature_low;
    uint8_t     lba_0;
    uint8_t     lba_1;
    uint8_t     lba_2;
    uint8_t     device_register;
    uint8_t     lba_3;
    uint8_t     lba_4;
    uint8_t     lba_5;
    uint8_t     feature_high;
    uint8_t     count_low;
    uint8_t     count_high;
    uint8_t     iso_command_completion;
    uint8_t     control;
    uint8_t     reserved_block[0x04];
} __attribute__((__packed__)) ahci_fis_reg_h2d_t;

typedef struct {
    ahci_hba_port_t*            port_hba;
    uint32_t                    port_number;
    ahci_controller_port_type_t port_type;
} ahci_drive_handle_t;

typedef struct {
    ahci_hba_mem_t*         hba_mem_abar;
    pci_dev_header_t*       pci_device;
    ahci_drive_handle_t*    drive_ports;
} ahci_bus_handle_t;

void ahci_drive_command_start(const ahci_drive_handle_t* drive);
void ahci_drive_command_stop(const ahci_drive_handle_t* drive);
bool ahci_drive_command_read_wait(const ahci_drive_handle_t* drive, uint64_t lba, uint32_t sector_count, void* out_buffer);

// Returns false if driver is not available, or there is no AHCI controller
const bool ahci_driver_available();
