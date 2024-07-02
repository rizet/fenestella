#include "iodev/disk/ahci.h"
#include "iodev/pci/confrw.h"
#include "iodev/uart/uartsh.h"
#include "mem/paging/paging.h"
#include "mem/pmm/pmm.h"
#include <string.h>

//
// Design largely ripped from Absurdponcho/PonchoOS (Public domain)
// However, severely modified due to serious bugs in original source
// Also, translated from C++ to C, and fit into this project's style
//

#define AHCI_PCI_DEVICE_CLASS           0x01
#define AHCI_PCI_DEVICE_SUBCLASS        0x06
#define AHCI_PCI_DEVICE_INTERFACE       0x01

#define HBA_PORT_DEV_DETECT_PRESENT     0x03
#define HBA_PORT_IPM_DETECT_ACTIVE      0x01

#define AHCI_SATA_SIGNATURE_ATAPI       0xEB140101
#define AHCI_SATA_SIGNATURE_ATA         0x00000101
#define AHCI_SATA_SIGNATURE_SEMB        0xC33C0101
#define AHCI_SATA_SIGNATURE_PM          0x96690101

#define AHCI_HBA_PxCMD_CR               0x8000
#define AHCI_HBA_PxCMD_FRE              0x0010
#define AHCI_HBA_PxCMD_ST               0x0001
#define AHCI_HBA_PxCMD_FR               0x4000

#define AHCI_ATA_DEV_BUSY               0x80
#define AHCI_ATA_DEV_DRQ                0x08

#define AHCI_ATA_CMD_READ_DMA_EXT       0x25

#define AHCI_CONTROLLER_PORT_COUNT      0x20
#define AHCI_CMD_HEADERS_PER_LIST       0x20


#define GET_CMD_HEADER_BASE(drive) \
    ((ahci_hba_command_header_t *)(((uint64_t)(drive)->port_hba->command_list_base_lower) | ((uint64_t)(drive)->port_hba->command_list_base_upper << 32)))
#define GET_CMD_TABLE_BASE(header) \
    ((ahci_hba_command_table_t *)((uintptr_t)(header)->command_table_base_lower | ((uintptr_t)(header)->command_table_base_upper << 32)))


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

void ahci_debug_print_port(const ahci_drive_handle_t* drive) {
    if (!ahci_driver_available()) {
        return;
    }

    if (drive->port_type != AHCI_CONTROLLER_PORT_TYPE_SATA) {
        serial_print_quiet("\r\nPort is an unsupported non-SATA port\r\n");
        return;
    }

    void* buffer = pmm_alloc_page();
    ahci_drive_command_read_wait(drive, 0, 4, buffer);

    char ch_buf[2] = { 0, 0 };
    serial_print_quiet("\r\n\n\n\n\n");
    for (uint32_t i = 0; i < 512; i++) {
        ch_buf[0] = ((char *)buffer)[i];
        serial_print_quiet(ch_buf);
    }
    serial_print_quiet("\r\n\n\n\n\n");
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
}

static bool __ports_configured = false;

void ahci_controller_configure_all_ports() {
    if (__ports_configured) {
        return;
    }
    ahci_controller_probe_ports();
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

bool ahci_drive_command_read_wait(const ahci_drive_handle_t* drive, uint64_t lba, uint32_t sector_count, void* out_buffer) {
    if (!ahci_driver_available()) {
        return false;
    }

    if (drive->port_type != AHCI_CONTROLLER_PORT_TYPE_SATA) {
        serial_print_error("Attempted to read from unsupported AHCI port!\r\n");
        return false;
    }

    uint32_t lba_lower = (uint32_t)(lba >> 00);
    uint32_t lba_upper = (uint32_t)(lba >> 32);

    drive->port_hba->interrupt_status = (uint32_t)-1;

    ahci_hba_command_header_t* cmd_header = GET_CMD_HEADER_BASE(drive);
    cmd_header->command_fis_length = sizeof(ahci_fis_reg_h2d_t) / sizeof(uint32_t);
    cmd_header->write_field = false;
    cmd_header->prdt_length = 1;

    ahci_hba_command_table_t* cmd_table = GET_CMD_TABLE_BASE(cmd_header);
    memset(cmd_table, 0, (sizeof(ahci_hba_command_table_t) + (cmd_header->prdt_length - 1) * sizeof(ahci_hba_prdt_entry_t)));

    cmd_table->prdt_entries[0].data_base_lower = (uint32_t)((uintptr_t)out_buffer >> 00);
    cmd_table->prdt_entries[0].data_base_upper = (uint32_t)((uintptr_t)out_buffer >> 32);
    cmd_table->prdt_entries[0].byte_count = (sector_count * 512);
    cmd_table->prdt_entries[0].interrupt_please = true;

    ahci_fis_reg_h2d_t* cmd_fis = (ahci_fis_reg_h2d_t *)(&cmd_table->command_fis);
    cmd_fis->fis_type = AHCI_FIS_TYPE_REG_H2D;
    cmd_fis->command_control = 1;
    cmd_fis->command = AHCI_ATA_CMD_READ_DMA_EXT;
    cmd_fis->lba_0 = (uint8_t)((lba_lower >>  0) & 0xFF);
    cmd_fis->lba_1 = (uint8_t)((lba_lower >>  8) & 0xFF);
    cmd_fis->lba_2 = (uint8_t)((lba_lower >> 16) & 0xFF);
    cmd_fis->lba_3 = (uint8_t)((lba_upper >>  0) & 0xFF);
    cmd_fis->lba_4 = (uint8_t)((lba_upper >>  8) & 0xFF);
    cmd_fis->lba_5 = (uint8_t)((lba_upper >> 16) & 0xFF);

#define CMD_FIS_REG_LBA_MODE    (1 << 6)
    cmd_fis->device_register = CMD_FIS_REG_LBA_MODE;

    cmd_fis->count_low = (sector_count & 0xFF);
    cmd_fis->count_high =(sector_count >> 8);

    uint64_t watchdog = 0;
    while (drive->port_hba->task_file_data & (AHCI_ATA_DEV_BUSY | AHCI_ATA_DEV_DRQ)) {
        if (watchdog++ > 0x100000) {
            return false;
        }
    }

#define HBA_STATUS_TFES     (1 << 30)
    drive->port_hba->command_issue = 1;

    while (true) {
        if (drive->port_hba->interrupt_status & HBA_STATUS_TFES) {
            return false;
        }
        if (drive->port_hba->command_issue == 0) {
            break;
        }
    }

    return true;
}

void __uartsh_disk_dump() {
    if (!ahci_driver_available()) {
        serial_print_error("AHCI driver not available\r\n");
        return;
    }

    serial_print_quiet("\r\n\n\nAHCI Disk Testing:\r\n");
    char itoa_buffer[67];
    for (uint32_t i = 0; i < AHCI_CONTROLLER_PORT_COUNT; i++) {
        ahci_drive_handle_t* drive = &_ahci_bus_handle.drive_ports[i];
        if (drive->port_hba == NULL || drive->port_type == AHCI_CONTROLLER_PORT_TYPE_NONE || drive->port_number != i) {
            continue;
        }

        serial_print_quiet("\r\n\n---\r\nPort ");
        serial_print_quiet(utoa(drive->port_number, itoa_buffer, 10));
        serial_print_quiet(", sector 0:\r\n");

        ahci_debug_print_port(drive);
    }
    serial_print_quiet("\r\n=====\r\n\n");
}

void __uartsh_disk_info() {
    serial_print_quiet("\r\n\n\nAHCI Disk Information:\r\nMay take up to a minute...\r\n");
    if (!ahci_driver_available()) {
        serial_print_error("AHCI driver not available\r\n\n\n");
        return;
    }

    char itoa_buffer[67];
    for (uint32_t i = 0; i < AHCI_CONTROLLER_PORT_COUNT; i++) {
        ahci_drive_handle_t* drive = &_ahci_bus_handle.drive_ports[i];
        if (drive->port_hba == NULL || drive->port_number != i) {
            continue;
        }

        serial_print_quiet("\r\n\tPort ");
        serial_print_quiet(utoa(drive->port_number, itoa_buffer, 10));
        serial_print_quiet(", class: ");
        switch (drive->port_type) {
            case AHCI_CONTROLLER_PORT_TYPE_SATA:
                serial_print_quiet("SATA");
                break;
            case AHCI_CONTROLLER_PORT_TYPE_SATAPI:
                serial_print_quiet("SATAPI");
                break;
            case AHCI_CONTROLLER_PORT_TYPE_SEMB:
                serial_print_quiet("SEMB");
                break;
            case AHCI_CONTROLLER_PORT_TYPE_PM:
                serial_print_quiet("PM");
                break;
            case AHCI_CONTROLLER_PORT_TYPE_NONE:
                serial_print_quiet("NONE");
                break;
            default:
                serial_print_quiet("ERROR");
                break;
        }
    }
    serial_print_quiet("\r\n\n");
}
