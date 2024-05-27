#include "iodev/disk/ahci.h"
#include "iodev/pci/confrw.h"
#include "mem/paging/paging.h"

#define AHCI_PCI_DEVICE_CLASS       0x01
#define AHCI_PCI_DEVICE_SUBCLASS    0x06
#define AHCI_PCI_DEVICE_INTERFACE   0x01

typedef enum {
    AHCI_CONTROLLER_PORT_TYPE_NONE,
    AHCI_CONTROLLER_PORT_TYPE_SATA,
    AHCI_CONTROLLER_PORT_TYPE_SEMB,
    AHCI_CONTROLLER_PORT_TYPE_PM,
    AHCI_CONTROLLER_PORT_TYPE_SATAPI,
} ahci_controller_port_type_t;

static bool _ahci_driver_loaded = false;
static bool _ahci_controller_checked = false;

static ahci_bus_handle_t _ahci_bus_handle = {
    .hba_mem_abar = NULL,
    .pci_device = NULL
};

const bool _ahci_driver_load_pci() {
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

    _ahci_controller_checked = true;
    return (paging_map_page((void *)_ahci_bus_handle.hba_mem_abar, (void *)_ahci_bus_handle.hba_mem_abar, PAGING_FLAGS_KERNEL_PAGE) == NULL);
}

const bool ahci_driver_available() {
    if (!_ahci_controller_checked) {
        _ahci_driver_loaded = _ahci_driver_load_pci();
    }
    return _ahci_driver_loaded;
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
        case AHCI_SATA_SIGNATURE_ATAPI:
            return AHCI_CONTROLLER_PORT_TYPE_SATAPI;
        case AHCI_SATA_SIGNATURE_ATA:
            return AHCI_CONTROLLER_PORT_TYPE_SATA;
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
}

void ahci_controller_probe_ports() {
    if (!ahci_driver_available()) {
        return;
    }

    uint32_t ports_implemented = _ahci_bus_handle.hba_mem_abar->ports_implemented;

    for (uint32_t i = 0; i < 32; i++) {
        if (ports_implemented & (1 << i)) {
            ahci_controller_register_port(i);
        }
    }
}
