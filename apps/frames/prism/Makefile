SOURCE	= src
OUTPUT	= bin

CCODE 	= $(shell find $(SOURCE) -type f -name '*.c')
SCODE	= $(shell find $(SOURCE) -type f -name '*.s')
OBJECTS	= $(patsubst $(SOURCE)/%.c, $(OUTPUT)/%.c.o, $(CCODE))
OBJECTS += $(patsubst $(SOURCE)/%.s, $(OUTPUT)/%.s.o, $(SCODE))

PROGRAM	= $(OUTPUT)/prism.se

CC 	= clang
LD 	= clang
AS 	= nasm

CCFLAGS = \
-std=c17 \
-ffreestanding \
-fno-stack-protector \
-I$(SOURCE) \
-mno-red-zone \
-fpic \
-fpie \
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

$(PROGRAM): $(OBJECTS)
	@ echo "	link $@ (1/1)"
	@ $(LD) $(LDFLAGS) $(OBJECTS) -o $(PROGRAM)

clean:
	@ rm -rf $(OUTPUT)

all: $(PROGRAM)

test:
	
