#include "lib/glass.h"
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>

typedef struct _heap_seg_hdr heap_seg_header_t;

struct _heap_seg_hdr {
    size_t length;
    heap_seg_header_t* next;
    heap_seg_header_t* last;
    bool free;
};

void* heap_start;
void* heap_end;
heap_seg_header_t* last_header;

heap_seg_header_t* header_split(heap_seg_header_t* segment, size_t split_length) {
    if (split_length < 0x10) return NULL;
    int64_t length = segment->length - split_length - (sizeof(heap_seg_header_t));
    if (length < 0x20) return NULL;

    heap_seg_header_t* new_header = (heap_seg_header_t *)((size_t)segment + split_length + sizeof(heap_seg_header_t));

    if (segment->next)
        segment->next->last = new_header;

    new_header->next = segment->next;
    segment->next = new_header;
    new_header->last = segment;
    new_header->length = length - sizeof(heap_seg_header_t);
    new_header->free = true;
    segment->length = split_length;

    if (last_header == segment) last_header = new_header;
    return new_header;
}

void header_combine_forward(heap_seg_header_t* segment) {
    if (segment->next == NULL) return;
    if (!segment->next->free) return;

    if (segment->next == last_header) last_header = segment;

    if (segment->next->next != NULL) {
        segment->next->next->last = segment;
    }

    segment->length = segment->length + segment->next->length + sizeof(heap_seg_header_t);

    memset((void *)segment->next, 0x00, sizeof(heap_seg_header_t));

    segment->next = segment->next->next;
}

void header_combine_backward(heap_seg_header_t* segment) {
    if (segment->last != NULL && segment->last->free) header_combine_forward(segment->last);
}

static bool heap_initialized = false;
void heap_initialize(void* address, size_t pages) {
    if (heap_initialized) return;
    void* pos = address;

    for (size_t i = 0; i < pages; i++) {
        pmap(pos);
        pos = (void*)((size_t)pos + 0x1000);
    }

    size_t length = pages * 0x1000;

    heap_start = address;
    heap_end = (void*)((size_t)heap_start + length);
    heap_seg_header_t* start = (heap_seg_header_t*)address;
    start->length = length - sizeof(heap_seg_header_t);
    start->next = NULL;
    start->last = NULL;
    start->free = true;
    last_header = start;

    heap_initialized = true;
}

void heap_expand(size_t length) {
    if (length % 0x1000) {
        length -= length % 0x1000;
        length += 0x1000;
    }

    size_t pages = length / 0x1000;
    heap_seg_header_t* new_segment = (heap_seg_header_t*)heap_end;

    for (size_t i = 0; i < pages; i++) {
        pmap(heap_end);
        heap_end = (void*)((size_t)heap_end + 0x1000);
    }

    new_segment->free = true;
    new_segment->last = last_header;
    last_header->next = new_segment;
    last_header = new_segment;
    new_segment->next = NULL;
    new_segment->length = length - sizeof(heap_seg_header_t);
    header_combine_backward(new_segment);
}

void free(void* address){
    heap_seg_header_t* segment = (heap_seg_header_t *)address - 1;
    if (((uint64_t)segment->next - segment->length) != (uint64_t)segment) {
        return;
    }
    segment->free = true;
    header_combine_forward(segment);
    header_combine_backward(segment);
}

void* malloc(size_t size) {
    if (!heap_initialized)
        heap_initialize((void *)0x7f0000000000, 256);

    if (size % 0x10 > 0) {
        size -= (size % 0x10);
        size += 0x10;
    }

    if (size == 0) return NULL;

    heap_seg_header_t* current = (heap_seg_header_t *)heap_start;

    while (true) {
        if (current->free) {
            if (current->length > size) {
                if (!header_split(current, size)) {
                    current->free = false;
                    return (void*)((uint64_t)current + sizeof(heap_seg_header_t));
                }
                current->free = false;
                return (void*)((uint64_t)current + sizeof(heap_seg_header_t));
            }
            if (current->length == size) {
                current->free = false;
                return (void*)((uint64_t)current + sizeof(heap_seg_header_t));
            }
        }
        if (current->next == NULL) break;
        current = current->next;
    }

    heap_expand(size);

    return malloc(size);
}

void* realloc(void* pointer, size_t size) {
    heap_seg_header_t* segment = (heap_seg_header_t *)pointer - 1;
    size_t copy_size = segment->length;

    void* new = malloc(size);

    memcpy(new, pointer, size);

    free(pointer);

    return new;
}

void* calloc(size_t elements, size_t size) {
    size_t real_size = elements * size;
    void* alloc = malloc(real_size);
    memset(alloc, 0x00, real_size);
    return alloc;
}
