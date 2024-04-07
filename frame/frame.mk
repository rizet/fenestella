SOURCE	= src
OUTPUT	= build

CCODE 	= $(shell find $(SOURCE) -type f -name '*.c')
SCODE	= $(shell find $(SOURCE) -type f -name '*.s')
OBJECTS	= $(patsubst $(SOURCE)/%.c, $(OUTPUT)/%.c.o, $(CCODE))
OBJECTS += $(patsubst $(SOURCE)/%.s, $(OUTPUT)/%.s.o, $(SCODE))

FRAME	= $(OUTPUT)/frame.se

CC 	= clang
LD 	= clang
AS 	= nasm

SYMGEN	= ./symbol_gen.sh
SYMS	= __symbols.c

CCFLAGS = \
-std=c17 \
-ffreestanding \
-fno-stack-protector \
-I$(SOURCE) \
-mno-red-zone \
-fpic \
-gdwarf \
-Werror \
-Wno-switch-bool

LDFLAGS = \
-nostdlib

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

$(FRAME): $(OBJECTS)
	@ echo "	link $@ (1/2)"
	@ $(LD) $(LDFLAGS) $(OBJECTS) -o $(FRAME)
	@ echo "	generate symbols..."
	@ chmod 777 $(SYMGEN)
	@ $(SYMGEN) $(FRAME)
	@ echo "	compile symbols..."
	@ $(CC) $(CCFLAGS) -c $(SYMS) -o $(OUTPUT)/$(SYMS:.c=.o)
	@ echo "	link $@ (2/2)"
	@ $(LD) $(LDFLAGS) $(OBJECTS) $(OUTPUT)/$(SYMS:.c=.o) -o $(FRAME)
	@ rm $(SYMS)

clean:
	@ rm -rf build

all: $(FRAME)