#pragma once
#include <stdint.h>

inline
__attribute__((always_inline)) 
void outb(uint16_t port, uint8_t val) {
    __asm__ volatile("outb %0, %1"
                :
                : "a"(val), "Nd"(port));
}

inline
__attribute__((always_inline)) 
void outc(uint16_t port, char val) {
    __asm__ volatile("outb %0, %1"
                :
                : "a"(val), "Nd"(port));
}

inline
__attribute__((always_inline)) 
uint8_t inb(uint16_t port) {
    uint8_t ret;
    __asm__ volatile("inb %1, %0"
                : "=a"(ret)
                : "Nd"(port));
    return ret;
}

inline
__attribute__((always_inline)) 
void outw(uint16_t port, uint16_t val) {
    __asm__ volatile("outw %0, %1"
                :
                : "a"(val), "Nd"(port));
}

inline
__attribute__((always_inline)) 
uint16_t inw(uint16_t port) {
    uint16_t ret;
    __asm__ volatile("inw %1, %0"
                : "=a"(ret)
                : "Nd"(port));
    return ret;
}

inline
__attribute__((always_inline)) 
void outl(uint16_t port, uint32_t val) {
    __asm__ volatile("outl %0, %1"
                :
                : "a"(val), "Nd"(port));
}

inline
__attribute__((always_inline)) 
uint32_t inl(uint16_t port) {
    uint32_t ret;
    __asm__ volatile("inl %1, %0"
                : "=a"(ret)
                : "Nd"(port));
    return ret;
}

inline
__attribute__((always_inline)) 
void wrmsr(uint64_t msr, uint64_t value) {
    uint32_t eax, edx;

    eax = (uint32_t)(value & 0xFFFFFFFF);
    edx = (uint32_t)(value >> 0x20);

    __asm__ volatile ("wrmsr" : : "a"(eax), "d"(edx), "c"(msr));
}

inline
__attribute__((always_inline)) 
uint64_t rdmsr(uint64_t msr) {
    uint32_t eax, edx;

    __asm__ volatile ("rdmsr" : "=a"(eax), "=d"(edx) : "c"(msr));

    uint64_t value = (uint64_t)eax;
    value |= ((uint64_t)edx << 0x20);

    return value;
}
