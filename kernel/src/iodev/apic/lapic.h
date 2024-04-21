#pragma once
#include "iodev/io.h"
#include "mem/paging/paging.h"
#include <stdint.h>

#define IA32_APIC_BASE	0x1B

typedef enum {
	APIC_LOCAL_REGISTER_ID					    = 0x0020,
	APIC_LOCAL_REGISTER_VERSION				    = 0x0030,
	APIC_LOCAL_REGISTER_TASK_PRIORITY			= 0x0080,
	APIC_LOCAL_REGISTER_ARBITRATION_PRIORITY	= 0x0090,
	APIC_LOCAL_REGISTER_PROCESSOR_PRIORITY	    = 0x00A0,
	APIC_LOCAL_REGISTER_EOI					    = 0x00B0,
	APIC_LOCAL_REGISTER_REMOTE_READ			    = 0x00C0,
	APIC_LOCAL_REGISTER_LOGICAL_DESTINATION	    = 0x00D0,
	APIC_LOCAL_REGISTER_DESTINATION_FORMAT	    = 0x00E0,
	APIC_LOCAL_REGISTER_SPURIOUS_INT_VECTOR	    = 0x00F0,
	APIC_LOCAL_REGISTER_IN_SERVICE			    = 0x0100,
	APIC_LOCAL_REGISTER_TRIGGER_MODE			= 0x0180,
	APIC_LOCAL_REGISTER_INTERRUPT_REQUEST		= 0x0200,
	APIC_LOCAL_REGISTER_ERROR_STATUS			= 0x0280,
	APIC_LOCAL_REGISTER_CMCI					= 0x02F0,
	APIC_LOCAL_REGISTER_INTERRUPT_COMMAND		= 0x0300,
	APIC_LOCAL_REGISTER_INTERRUPT_DESTINATION	= 0x0310,
	APIC_LOCAL_REGISTER_LVT_TIMER				= 0x0320,
	APIC_LOCAL_REGISTER_LVT_THERMAL_SENSOR	    = 0x0330,
	APIC_LOCAL_REGISTER_LVT_PMC					= 0x0340,
	APIC_LOCAL_REGISTER_LVT_LINT0				= 0x0350,
	APIC_LOCAL_REGISTER_LVT_LINT1				= 0x0360,
	APIC_LOCAL_REGISTER_LVT_ERROR				= 0x0370,
	APIC_LOCAL_REGISTER_INITIAL_COUNT			= 0x0380,
	APIC_LOCAL_REGISTER_CURRENT_COUNT			= 0x0390,
	APIC_LOCAL_REGISTER_DIVIDE_CONFIG			= 0x03E0
} apic_local_register_t;

extern uint32_t* apic_virtual_base;

static
inline void apic_local_set_base(void* base) {
	base = (void *)((uint64_t)base & 0xffffffff);
	wrmsr(IA32_APIC_BASE, (uint64_t)base & 0xfffff000);
	apic_virtual_base = (uint32_t *)paging_map_page((void *)((uint64_t)base + PAGING_VIRTUAL_OFFSET), base, PAGING_FLAGS_KERNEL_PAGE);
}

static
inline void apic_local_write(apic_local_register_t local_register, uint32_t value) {
	uint32_t volatile* destination = (uint32_t *)((uint64_t)apic_virtual_base + local_register);
	*destination = value;
}

static
inline uint32_t apic_local_read(apic_local_register_t local_register) {
	uint32_t volatile* destination = (uint32_t *)((uint64_t)apic_virtual_base + local_register);
	return *destination;
}

void	apic_local_send_eoi(void);

void	apic_local_send_ipi(uint8_t lapic_id, uint8_t vector);
void 	apic_local_cast_ipi(uint8_t vector);
