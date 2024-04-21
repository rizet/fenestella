#include "proc/loader/elf.h"
#include "mem/pmm/pmm.h"
#include "mem/paging/paging.h"
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>

bool elf_is_page_mapped(elf_load_info_t* info, uint64_t p_vaddr) {
    for (uint64_t i = 0; i < info->segment_count; i++)
        if ((info->segments[i].loaded_at <= p_vaddr && info->segments[i].loaded_at + info->segments[i].length > p_vaddr))
            return true;

    return false;
}

bool elf_load_segment(void* file, elf_program_header_t* header, elf_load_info_t* out_info) {
    if (header->p_memsz == 0)
        return true;

    elf_program_header_t header_copy = *header;

    // copy data into already mapped pages
    while (elf_is_page_mapped(out_info, header_copy.p_vaddr)) {
        uint64_t page_offset = header_copy.p_vaddr % PAGING_PAGE_SIZE;
        uint64_t next_page_offset = PAGING_PAGE_SIZE - page_offset;
        if (next_page_offset > header_copy.p_filesz)
            next_page_offset = header_copy.p_filesz;

        memcpy((void *)header_copy.p_vaddr, (void *)((uint64_t)file + header_copy.p_offset), next_page_offset);
        header_copy.p_vaddr += next_page_offset;
        header_copy.p_offset += next_page_offset;
        header_copy.p_filesz -= next_page_offset;
        header_copy.p_memsz -= next_page_offset;

        if (header_copy.p_memsz == 0)
            return true;
    }
    // now all data in mapped pages is handled, p_vaddr IS in an unmapped page
    uint64_t required_pages = (header_copy.p_memsz / PAGING_PAGE_SIZE) + (header_copy.p_memsz % PAGING_PAGE_SIZE == 0 ? 0 : 1);
    if (header_copy.p_vaddr % PAGING_PAGE_SIZE != 0) required_pages++;
    void* real_location = pmm_alloc_pool(required_pages);

    uint64_t page_offset = header_copy.p_vaddr & 0xfff;

    out_info->segments[out_info->segment_count].loaded_at = header_copy.p_vaddr;
    out_info->segments[out_info->segment_count].located_at = (uint64_t)real_location;
    out_info->segments[out_info->segment_count].length = required_pages * PAGING_PAGE_SIZE;
    out_info->segment_count++;

    // do the actual data copy
    memset((void *)((uint64_t)real_location + page_offset), 0, header_copy.p_memsz);
    memcpy((void *)((uint64_t)real_location + page_offset), (void *)((uint64_t)file + header_copy.p_offset), header_copy.p_filesz);

    return true;
}

elf_load_info_t* elf_load_program(void* file) {
    elf_header_t* header = (elf_header_t *)file;

    if (strncmp((char *)&header->magic[0], ELF_HEADER_MAGIC, 4) > 0)
        return NULL;

    if (header->architecture != ELF_ARCH_x86_64)
        return NULL;

    if (header->endianness != ELF_ENDIANNESS_LITTLE || header->bitness != ELF_BITNESS_64)
        return NULL;

    if (header->abi != ELF_ABI_SYSTEMV)
        return NULL;

    size_t header_table_size = header->program_table_entry_size * header->program_table_entries;

    elf_load_info_t* load_info = (elf_load_info_t *)malloc(sizeof(elf_load_info_t));
    load_info->entry = header->program_entry;
    load_info->segment_count = 0;
    load_info->segments = (elf_loaded_page_t *)pmm_alloc_page();

    for (elf_program_header_t* pheader = (elf_program_header_t *)(file + header->header_table);;) {
        if (((uint64_t)pheader - ((uint64_t)file + header->header_table)) >= header_table_size)
            break;

        if (pheader->type == ELF_SEGMENT_LOAD)
            if (!elf_load_segment(file, pheader, load_info))
                return NULL;

        pheader = (elf_program_header_t *)((void *)pheader + header->program_table_entry_size);
    }
    
    return load_info;
}
