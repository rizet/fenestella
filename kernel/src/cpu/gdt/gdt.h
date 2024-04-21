#pragma once
#include <stdint.h>

#define GDT_MAX_DESCRIPTORS         0x2000
#define GDT_DESCRIPTOR_SIZE         0x08

#define GDT_DESCRIPTOR_ACCESS       0x01
#define GDT_DESCRIPTOR_READWRITE    0x02
#define GDT_DESCRIPTOR_DC           0x04
#define GDT_DESCRIPTOR_EXECUTABLE   0x08
#define GDT_DESCRIPTOR_CODE_DATA    0x10
#define GDT_DESCRIPTOR_DPL          0x60
#define GDT_DESCRIPTOR_PRESENT      0x80

#define GDT_GRANULARITY_OS          0x10
#define GDT_GRANULARITY_X64         0x20
#define GDT_GRANULARITY_X32         0x40
#define GDT_GRANULARITY_4K          0x80

#define GDT_BASIC_DESCRIPTOR        (GDT_DESCRIPTOR_PRESENT | GDT_DESCRIPTOR_READWRITE | GDT_DESCRIPTOR_CODE_DATA)
#define GDT_BASIC_GRANULARITY       (GDT_GRANULARITY_X64 | GDT_GRANULARITY_4K)

#define GDT_OFFSET_KERNEL_CODE      (0x01 * 0x08)
#define GDT_OFFSET_KERNEL_DATA      (0x02 * 0x08)
#define GDT_OFFSET_USER_DATA        (0x03 * 0x08)
#define GDT_OFFSET_USER_CODE        (0x04 * 0x08)

typedef struct {
    uint16_t    limit;         
    uint16_t    base_low;      
    uint8_t     base_mid;       
    uint8_t     flags;          
    uint8_t     granularity;    
    uint8_t     base_high;      
} __attribute__((packed)) gdt_desc_t;

typedef struct {
    uint16_t    limit_0;
    uint16_t    addr_0;
    uint8_t     addr_1;
    uint8_t     type_0;
    uint8_t     limit_1;
    uint8_t     addr_2;
    uint32_t    addr_3;
    uint32_t    reserved;
} __attribute__((packed)) gdt_tss_desc_t;
        
typedef struct {
    uint16_t limit;         
    uintptr_t base;       
} __attribute__((packed)) gdtr_t;

void        gdt_add_descriptor(uint64_t base, uint16_t limit, uint8_t access, uint8_t granularity);
void        gdt_reload(gdtr_t* gdtr, uint16_t code, uint16_t data);
uint16_t    gdt_install_tss(uint64_t tss);
void        gdt_assemble(void);
