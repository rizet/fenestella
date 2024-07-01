#include "iodev/disk/ahci.h"
#include "iodev/pci/confrw.h"
#include "mem/paging/paging.h"
#include "mem/pmm/pmm.h"
#include <string.h>

//
// Design largely ripped from Absurdponcho/PonchoOS (Public domain)
// However, severely modified due to serious bugs in original source
// Also, translated from C++ to C, and fit into this project's style
//

#define AHCI_PCI_DEVICE_CLASS       0x01
#define AHCI_PCI_DEVICE_SUBCLASS    0x06
#define AHCI_PCI_DEVICE_INTERFACE   0x01

#define AHCI_HBA_PxCMD_CR   0x8000
#define AHCI_HBA_PxCMD_FRE  0x0010
#define AHCI_HBA_PxCMD_ST   0x0001
#define AHCI_HBA_PxCMD_FR   0x4000

#define AHCI_CONTROLLER_PORT_COUNT  0x20
#define AHCI_CMD_HEADERS_PER_LIST   0x20

static bool _ahci_driver_loaded = false;
static bool _ahci_controller_checked = false;

static ahci_bus_handle_t _ahci_bus_handle = {
    .hba_mem_abar = NULL,
    .pci_device = NULL,
    .drive_ports = NULL
};

void ahci_controller_configure_all_ports();

const bool _ahci_driver_load_pci() {
    if (_ahci_controller_checked) {
        return _ahci_driver_loaded;
    }

    _ahci_driver_loaded = false;
    _ahci_bus_handle.pci_device = find_device(AHCI_PCI_DEVICE_CLASS, AHCI_PCI_DEVICE_SUBCLASS, AHCI_PCI_DEVICE_INTERFACE)->base;
    if (_ahci_bus_handle.pci_device == NULL) {
        _ahci_controller_checked = true;
        return false;
    }

    _ahci_bus_handle.hba_mem_abar = (ahci_hba_mem_t *)((uintptr_t)((pci_ext_header_0_t *)_ahci_bus_handle.pci_device)->bar5);
    if (_ahci_bus_handle.hba_mem_abar == NULL) {
        _ahci_controller_checked = true;
        return false;
    }

    _ahci_bus_handle.drive_ports = (ahci_drive_handle_t *)pmm_alloc_page();
    memset(_ahci_bus_handle.drive_ports, 0, sizeof(ahci_drive_handle_t) * AHCI_CONTROLLER_PORT_COUNT);

    _ahci_controller_checked = true;
    _ahci_driver_loaded = (paging_map_page((void *)_ahci_bus_handle.hba_mem_abar, (void *)_ahci_bus_handle.hba_mem_abar, PAGING_FLAGS_MMIO_PAGE) != NULL);

    ahci_controller_configure_all_ports();
    return _ahci_driver_loaded;
}

const bool ahci_driver_available() {
    return _ahci_driver_load_pci() && _ahci_driver_loaded;
}

#define HBA_PORT_DEV_DETECT_PRESENT     0x03
#define HBA_PORT_IPM_DETECT_ACTIVE      0x01
#define AHCI_SATA_SIGNATURE_ATAPI       0xEB140101
#define AHCI_SATA_SIGNATURE_ATA         0x00000101
#define AHCI_SATA_SIGNATURE_SEMB        0xC33C0101
#define AHCI_SATA_SIGNATURE_PM          0x96690101

ahci_controller_port_type_t ahci_controller_classify_port(const ahci_hba_port_t* port) {
    uint32_t sata_status = port->sata_status;
    uint32_t sata_signature = port->signature;

    uint8_t interface_power_management = (sata_status >> 8) & 0b111;
    uint8_t device_detection = sata_status & 0b111;

    if (device_detection != HBA_PORT_DEV_DETECT_PRESENT) {
        return AHCI_CONTROLLER_PORT_TYPE_NONE;
    } else if (interface_power_management != HBA_PORT_IPM_DETECT_ACTIVE) {
        return AHCI_CONTROLLER_PORT_TYPE_NONE;
    }

    switch (sata_signature) {
        case AHCI_SATA_SIGNATURE_ATA:
            return AHCI_CONTROLLER_PORT_TYPE_SATA;
        case AHCI_SATA_SIGNATURE_ATAPI:
            return AHCI_CONTROLLER_PORT_TYPE_SATAPI;
        case AHCI_SATA_SIGNATURE_SEMB:
            return AHCI_CONTROLLER_PORT_TYPE_SEMB;
        case AHCI_SATA_SIGNATURE_PM:
            return AHCI_CONTROLLER_PORT_TYPE_PM;
        default:
            return AHCI_CONTROLLER_PORT_TYPE_NONE;
    }
}

void ahci_controller_register_port(const uint32_t port) {
    ahci_hba_port_t* port_handle = &_ahci_bus_handle.hba_mem_abar->ports[port];
    ahci_controller_port_type_t port_type = ahci_controller_classify_port(port_handle);

    switch (port_type) {
        case AHCI_CONTROLLER_PORT_TYPE_SATA:
        case AHCI_CONTROLLER_PORT_TYPE_SATAPI:
            break;
        default:
            return;
    }

    _ahci_bus_handle.drive_ports[port].port_number = port;
    _ahci_bus_handle.drive_ports[port].port_type = port_type;
    _ahci_bus_handle.drive_ports[port].port_hba = port_handle;
}

void ahci_controller_probe_ports() {
    if (!ahci_driver_available()) {
        return;
    }

    uint32_t ports_implemented = _ahci_bus_handle.hba_mem_abar->ports_implemented;

    for (uint32_t i = 0; i < AHCI_CONTROLLER_PORT_COUNT; i++) {
        if (ports_implemented & (1 << i)) {
            ahci_controller_register_port(i);
        }
    }
}

void ahci_controller_configure_port(uint32_t port_no) {
    if (!ahci_driver_available()) {
        return;
    }

    ahci_drive_handle_t* drive = &_ahci_bus_handle.drive_ports[port_no];
    if (drive->port_hba == NULL || drive->port_type == AHCI_CONTROLLER_PORT_TYPE_NONE || drive->port_number != port_no) {
        return;
    }

    ahci_drive_command_stop(drive);

    void* new_cmd_base = pmm_alloc_page();
    void* new_fis_base = pmm_alloc_page();
    memset(new_cmd_base, 0, PAGING_PAGE_SIZE);
    memset(new_fis_base, 0, PAGING_PAGE_SIZE);
    drive->port_hba->command_list_base_lower = (uint32_t)((uintptr_t)new_cmd_base >> 00);
    drive->port_hba->command_list_base_upper = (uint32_t)((uintptr_t)new_cmd_base >> 32);
    drive->port_hba->fis_base_lower = (uint32_t)((uintptr_t)new_fis_base >> 00);
    drive->port_hba->fis_base_upper = (uint32_t)((uintptr_t)new_fis_base >> 32);

#define DESIRED_PRDT_LENGTH 8

    ahci_hba_command_header_t* cmd_header = (ahci_hba_command_header_t *)new_cmd_base;

    size_t cmd_table_size = sizeof(ahci_hba_command_table_t) + ((sizeof(ahci_hba_prdt_entry_t) * DESIRED_PRDT_LENGTH));
    void* cmd_tables_addr = pmm_alloc_pool(cmd_table_size / PAGING_PAGE_SIZE);
    memset(cmd_tables_addr, 0x00, cmd_table_size * AHCI_CMD_HEADERS_PER_LIST);

    for (uint32_t i = 0; i < AHCI_CMD_HEADERS_PER_LIST; i++) {
        cmd_header[i].prdt_length = DESIRED_PRDT_LENGTH;

        uint64_t cmd_table_base = (uint64_t)cmd_tables_addr + (i * cmd_table_size);
        cmd_header[i].command_table_base_lower = (uint32_t)(cmd_table_base >> 00);
        cmd_header[i].command_table_base_upper = (uint32_t)(cmd_table_base >> 32);
    }

    ahci_drive_command_start(drive);

    drive->data_buffer = pmm_alloc_page();
    memset(drive->data_buffer, 0, PAGING_PAGE_SIZE);
}

static bool __ports_configured = false;

void ahci_controller_configure_all_ports() {
    if (!__ports_configured) {
        return;
    }
    for (uint32_t i = 0; i < AHCI_CONTROLLER_PORT_COUNT; i++) {
        ahci_controller_configure_port(i);
    }
    __ports_configured = true;
}

void ahci_drive_command_start(const ahci_drive_handle_t* drive) {
    if (!ahci_driver_available()) {
        return;
    }

    while (drive->port_hba->command_status & AHCI_HBA_PxCMD_CR) {
        __asm__ volatile ("hlt");
    }

    drive->port_hba->command_status |= AHCI_HBA_PxCMD_FRE;
    drive->port_hba->command_status |= AHCI_HBA_PxCMD_ST;
}

void ahci_drive_command_stop(const ahci_drive_handle_t* drive) {
    if (!ahci_driver_available()) {
        return;
    }

    drive->port_hba->command_status &= ~AHCI_HBA_PxCMD_ST;
    drive->port_hba->command_status &= ~AHCI_HBA_PxCMD_FRE;

    while (true) {
        __asm__ volatile ("hlt");
        if (drive->port_hba->command_status & AHCI_HBA_PxCMD_FR) 
            continue;
        if (drive->port_hba->command_status & AHCI_HBA_PxCMD_CR) 
            continue;

        break;
    }
}
