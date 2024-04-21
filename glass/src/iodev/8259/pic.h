#pragma once
#include <stdint.h>

#define PIC_MASTER          0x20
#define PIC_SLAVE		    0xA0
#define PIC_MASTER_COMMAND	PIC_MASTER
#define PIC_MASTER_DATA	    (PIC_MASTER+1)
#define PIC_SLAVE_COMMAND	PIC_SLAVE
#define PIC_SLAVE_DATA	    (PIC_SLAVE+1)
#define PIC_EOI		        0x20

#define PIC_ICW1_ICW4	    0x01
#define PIC_ICW1_SINGLE	    0x02
#define PIC_ICW1_INTERVAL4  0x04
#define PIC_ICW1_LEVEL	    0x08
#define PIC_ICW1_INIT	    0x10

#define PIC_ICW4_8086	    0x01
#define PIC_ICW4_AUTO	    0x02
#define PIC_ICW4_BUF_SLAVE  0x08
#define PIC_ICW4_BUF_MASTER 0x0C

void pic_disable(void);
void pic_remap_offsets(uint8_t irq_offset);
void pic_mask_irq(uint8_t irq);
void pic_unmask_irq(uint8_t irq);
void pic_send_eoi(uint8_t irq);
