SOURCE	= src
OUTPUT	= build

CCODE 	= $(shell find $(SOURCE) -type f -name '*.c')
SCODE	= $(shell find $(SOURCE) -type f -name '*.s')
OBJECTS	= $(patsubst $(SOURCE)/%.c, $(OUTPUT)/%.c.o, $(CCODE))
OBJECTS += $(patsubst $(SOURCE)/%.s, $(OUTPUT)/%.s.o, $(SCODE))

LDS = glass.ld

GLASS	= $(OUTPUT)/glass.sys

CC 	= clang
LD 	= clang
AS 	= nasm

SYMGEN	= ./symbol_gen.sh
SYMS	= __symbols.c

CCFLAGS = \
-std=c17 \
-ffreestanding \
-I$(SOURCE) \
-mno-red-zone \
-mcmodel=kernel \
-fno-stack-protector \
-fpic \
-fpie \
-gdwarf \
-fwrapv \
-Werror \
-pipe

LDFLAGS = \
-nostdlib \
-Wl,-T$(LDS) \
-mcmodel=kernel \
-pie

ASFLAGS = \
-f elf64 \
-g \
-F dwarf

.DEFAULT-GOAL	= all
.PHONY			= clean

$(OUTPUT)/%.c.o: $(SOURCE)/%.c
	@ echo "	compile $^"
	@ mkdir -p $(@D)
	@ $(CC) $(CCFLAGS) -c $^ -o $@

$(OUTPUT)/%.s.o: $(SOURCE)/%.s
	@ echo "	assemble $^"
	@ mkdir -p $(@D)
	@ $(AS) $(ASFLAGS) $^ -o $@

$(GLASS): $(OBJECTS)
	@ echo "	link $@ (1/2)"
	@ $(LD) $(LDFLAGS) $(OBJECTS) -o $(GLASS)
	@ echo "	generate symbols..."
	@ chmod 777 $(SYMGEN)
	@ $(SYMGEN) $(GLASS)
	@ echo "	compile symbols..."
	@ $(CC) $(CCFLAGS) -c $(SYMS) -o $(OUTPUT)/$(SYMS:.c=.o)
	@ echo "	link $@ (2/2)"
	@ $(LD) $(LDFLAGS) $(OBJECTS) $(OUTPUT)/$(SYMS:.c=.o) -o $(GLASS)
	@ rm $(SYMS)

clean:
	@ rm -rf build

all: $(GLASS)
