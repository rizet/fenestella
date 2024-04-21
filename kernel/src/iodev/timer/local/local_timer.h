#pragma once
#include <stdint.h>

#define LOCAL_TIMER_DIVISOR_002 0b0000
#define LOCAL_TIMER_DIVISOR_004 0x0001
#define LOCAL_TIMER_DIVISOR_008 0b0010
#define LOCAL_TIMER_DIVISOR_016 0b0011
#define LOCAL_TIMER_DIVISOR_032 0b1000
#define LOCAL_TIMER_DIVISOR_064 0b1001
#define LOCAL_TIMER_DIVISOR_128 0b1010
#define LOCAL_TIMER_DIVISOR_001 0b1011

void local_timer_calibrate();
void local_timer_set_frequency(uint64_t hz);

void local_timer_set_handler(void (*handler));

uint64_t local_timer_get_tpms();
