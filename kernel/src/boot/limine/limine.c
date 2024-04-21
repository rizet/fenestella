#include "boot/protocol.h"
#include "boot/limine/limine.h"
#include "iodev/acpi/tables/tables.h"
#include "mem/paging/paging.h"
#include "mem/pmm/pmm.h"
#include <string.h>

memory_map_t* memory_map;
boot_module_t* boot_modules;

extern void _start_limine64();

// Limine requests

struct limine_framebuffer_request l_framebuffer_req = {
    .id = LIMINE_FRAMEBUFFER_REQUEST,
    .revision = 0,
    .response = NULL
};
__attribute__((section(".limine_reqs")))
void* __framebuffer_req = (void *)&l_framebuffer_req;

struct limine_memmap_request l_memory_map_req = {
    .id = LIMINE_MEMMAP_REQUEST,
    .revision = 0,
    .response = NULL
};
__attribute__((section(".limine_reqs")))
void* __memory_map_req = (void *)&l_memory_map_req;

struct limine_module_request l_module_req = {
    .id = LIMINE_MODULE_REQUEST,
    .revision = 0,
    .response = NULL
};
__attribute__((section(".limine_reqs")))
void* __module_req = (void *)&l_module_req;

struct limine_rsdp_request l_rsdp_req = {
    .id = LIMINE_RSDP_REQUEST,
    .revision = 0,
    .response = NULL
};
__attribute__((section(".limine_reqs")))
void* __rsdp_req = (void *)&l_rsdp_req;

struct limine_stack_size_request l_stack_size_req = {
    .id = LIMINE_STACK_SIZE_REQUEST,
    .revision = 0,
    .response = NULL,
    .stack_size = PAGING_PAGE_SIZE * 16
};
__attribute__((section(".limine_reqs")))
void* __stack_size_req = (void *)&l_stack_size_req;

struct limine_hhdm_request l_hhdm_req = {
    .id = LIMINE_HHDM_REQUEST,
    .revision = 0,
    .response = NULL
};
__attribute__((section(".limine_reqs")))
void* __hhdm_req = (void *)&l_hhdm_req;

struct limine_entry_point_request l_entry_req = {
    .id = LIMINE_ENTRY_POINT_REQUEST,
    .revision = 0,
    .response = NULL,
    .entry = &_start_limine64
};
__attribute__((section(".limine_reqs")))
void* __entry_req = (void *)&l_entry_req;

struct limine_kernel_address_request l_address_req = {
    .id = LIMINE_KERNEL_ADDRESS_REQUEST,
    .revision = 0,
    .response = NULL
};
__attribute__((section(".limine_reqs")))
void* __address_req = (void *)&l_address_req;

__attribute__((section(".limine_reqs")))
void* __final_req = NULL;

static uint64_t _kernel_physical_load;
static uint64_t _kernel_virtual_load;

void* get_kernel_load_physical() {
    return (void *)_kernel_physical_load;
}
uint64_t get_kernel_virtual_offset() {
    return _kernel_virtual_load - _kernel_physical_load;
}

// End Limine requests

boot_module_t* get_boot_module(char* name) {
    if (strlen(name) > 128)
        return NULL;

    for (uint64_t i = 0; i < boot_module_count; i++) {
        if (!strcmp(name, boot_modules[i].name))
            return &boot_modules[i];
    }

    return NULL;
}

#define IOMMU_IGNORED

uint64_t limine_mmap_type_convert(uint64_t limine_type) {
    switch (limine_type) {
        case LIMINE_MEMMAP_USABLE:
            return MEMORY_MAP_FREE;
        case LIMINE_MEMMAP_RESERVED:
            return MEMORY_MAP_BUSY;
        case LIMINE_MEMMAP_KERNEL_AND_MODULES:
            return MEMORY_MAP_BUSY;
        case LIMINE_MEMMAP_FRAMEBUFFER:
            return MEMORY_MAP_MMIO;
        case LIMINE_MEMMAP_ACPI_NVS:
            return MEMORY_MAP_NOUSE;
        case LIMINE_MEMMAP_BAD_MEMORY:
            return MEMORY_MAP_NOUSE;
        case LIMINE_MEMMAP_BOOTLOADER_RECLAIMABLE:
            return MEMORY_MAP_BUSY;
        case LIMINE_MEMMAP_ACPI_RECLAIMABLE:
            return MEMORY_MAP_BUSY;
        default:
            return MEMORY_MAP_NOUSE;
    }
}

void limine_reinterpret();
void limine_reinterpret() {
    acpi_load_rsdp(l_rsdp_req.response->address);

    // dark blood magic
    // find two consecutive entries USED->FREE
    // use a few pages from the beginning of the free section
    // modify the memory map to protect section from pmm 
    // could break if memory map is too big in a small section
    
    // pointer to array of pointers to entries
    struct limine_memmap_entry** limine_map = l_memory_map_req.response->entries;
    uint64_t limine_map_entries = l_memory_map_req.response->entry_count;

    _kernel_physical_load = l_address_req.response->physical_base;
    _kernel_virtual_load = l_address_req.response->virtual_base;

    uint64_t map_pages = (limine_map_entries * sizeof(struct limine_memmap_entry)) / PAGING_PAGE_SIZE;  
    if (((limine_map_entries * sizeof(struct limine_memmap_entry)) % PAGING_PAGE_SIZE) != 0)
        map_pages++;

    map_pages+=2; // modules stuff too

    struct limine_memmap_entry* used_entry = NULL;
    struct limine_memmap_entry* free_entry = NULL;

    
    #ifdef IOMMU_IGNORED
    // pre-parse, check for the massive iommu entry
    for (uint64_t i = 0; i < limine_map_entries; i++) {
        if (limine_map[i]->base >= MEMORY_AMD_IOMMU_BLOCK_START && limine_map[i]->base <= MEMORY_AMD_IOMMU_BLOCK_END) {
            limine_map[i]->type = LIMINE_MEMMAP_BAD_MEMORY;
        }
    }
    #endif

    for (uint64_t i = 0; i < limine_map_entries; i++) {
        if (limine_map[i+1]->type == LIMINE_MEMMAP_USABLE 
                && (limine_mmap_type_convert(limine_map[i]->type) == MEMORY_MAP_BUSY)
                && ((limine_map[i+1]->length / PAGING_PAGE_SIZE) > map_pages)) 
        {
            used_entry = (struct limine_memmap_entry *)limine_map[i];
            free_entry = (struct limine_memmap_entry *)limine_map[i+1];
            break;
        }
    }

    if (!used_entry || !free_entry) {
        __asm__ volatile ("cli");
        __asm__ volatile ("hlt");
    }

    void* candidate = (void *)free_entry->base;
    used_entry->length += (map_pages * PAGING_PAGE_SIZE);
    free_entry->base += (map_pages * PAGING_PAGE_SIZE);
    free_entry->length -= (map_pages * PAGING_PAGE_SIZE);
    // I'm imagining a possible scenario where the memory map is protected under an ACPI NVS region...

    // Now we actually have to make the new memory map
    memory_map = (memory_map_t *)candidate;
    memory_map->entries = (memory_map_entry_t *)((uint64_t)candidate + sizeof(memory_map_t));
    memory_map->entry_count = limine_map_entries;

    for (uint64_t i = 0; i < memory_map->entry_count; i++) {
        memory_map->entries[i].base = limine_map[i]->base;
        memory_map->entries[i].length = limine_map[i]->length;
        memory_map->entries[i].signal = limine_mmap_type_convert(limine_map[i]->type);
    }

    // Make first megabyte unusable
    memory_map->entries[0].base = 0x0000000000000000;
    memory_map->entries[0].length = 0x0000000000100000;
    memory_map->entries[0].signal = MEMORY_MAP_NOUSE;
    for (uint64_t i = 1; i < memory_map->entry_count; i++) {
        if (memory_map->entries[i].base < 0x0000000000100000) {
            if (memory_map->entries[i].base + memory_map->entries[i].length < 0x0000000000100000) {
                memory_map->entries[i].length = 0;
                memory_map->entries[i].signal = MEMORY_MAP_NOUSE;
            }
            else {
                memory_map->entries[i].length -= (0x0000000000100000 - memory_map->entries[i].base);
                memory_map->entries[i].base = 0x0000000000100000;
            }
        }
        else break;
    }

    // assumes 32-bit framebuffer...
    framebuffer.frame_addr = (uint64_t)l_framebuffer_req.response->framebuffers[0]->address;
    framebuffer.frame_height = l_framebuffer_req.response->framebuffers[0]->height;
    framebuffer.frame_width = l_framebuffer_req.response->framebuffers[0]->width;
    framebuffer.frame_pitch = l_framebuffer_req.response->framebuffers[0]->pitch;
    framebuffer.frame_bpp = l_framebuffer_req.response->framebuffers[0]->bpp;
    
    // Hope modules aren't tooo many (although I ultimately have control over this)
    boot_modules = (boot_module_t *)((uint64_t)candidate + ((map_pages - 2) * PAGING_PAGE_SIZE));
    boot_module_count = l_module_req.response->module_count;

    for (uint64_t i = 0; i < boot_module_count; i++) {     
        boot_modules[i].name = l_module_req.response->modules[i]->cmdline;
        boot_modules[i].virt = (uint64_t)l_module_req.response->modules[i]->address;
        boot_modules[i].phys = (uint64_t)l_module_req.response->modules[i]->address & 0xFFFFFFFF;
        boot_modules[i].size = l_module_req.response->modules[i]->size;
    }
}

framebuffer_info_t framebuffer;
uint64_t boot_module_count;
