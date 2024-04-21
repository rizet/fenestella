ENTRY(_start_limine64)
OUTPUT_FORMAT(elf64-x86-64)

SECTIONS
{
	. = 0xffffffff80000000 + 2M;

	__load_base = .;

	.limine_reqs : {
        KEEP(*(.limine_reqs))
    }

	.text : ALIGN(0x1000) {
		*(.text*)
	}

	.data : ALIGN(0x1000) {
		*(.data*)
	}

	.rodata : ALIGN(0x1000) {
		*(.rodata*)
	}

	.bss : ALIGN(0x1000) {
		*(COMMON)
		*(.bss*)
	}
	
	. = ALIGN(0x1000);
	
	__load_max = .;
}