#pragma once
#include <stdint.h>

#define PIT_REGISTER_CHANNEL0_DATA  0x40
#define PIT_REGISTER_COMMAND_MODE   0x43

#define PIT_FREQUENCY               1193182

#define PIT_IRQ_LINE                0

extern uint8_t pit_vector;
extern uint16_t pit_divisor;

void pit_enable();
void pit_disable();
void pit_set_divisor(uint16_t divisor);

void pit_stopwatch_start();
uint64_t pit_stopwatch_stop();

void pit_deadline_wait(uint64_t delay_ticks);
