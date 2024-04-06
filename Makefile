OUTPUT	= build
GLASS	= $(OUTPUT)/glass.sys
FRAME	= $(OUTPUT)/frame.se
IMAGE	= $(OUTPUT)/skylight.hdd

LIMINE-EFI		= https://github.com/limine-bootloader/limine/raw/v7.x-binary/BOOTX64.EFI
# actually 7.0 somehow

.DEFAULT-GOAL	= image
.PHONY			= clean

$(GLASS):
	@ make -C glass -j10
	@ mkdir -p build
	@ cp glass/build/glass.sys $(GLASS)

$(FRAME):
	@ make -C frame -j10
	@ mkdir -p build
	@ cp frame/build/frame.se $(FRAME)

$(IMAGE): glass frame
	@ wget $(LIMINE-EFI) --quiet
	@ dd if=/dev/zero of=fat.img bs=1M count=128
	@ mformat -i fat.img -F ::
	@ mmd -i fat.img ::/efi
	@ mmd -i fat.img ::/efi/boot
	@ mmd -i fat.img ::/boot
	@ mmd -i fat.img ::/sys
	@ mmd -i fat.img ::/sys/start
	@ mmd -i fat.img ::/sys/share
	@ mcopy -i fat.img BOOTX64.EFI ::/EFI/BOOT/BOOTX64.EFI
	@ mcopy -i fat.img $(GLASS) ::/sys/start/glass.sys
	@ mcopy -i fat.img $(FRAME) ::/sys/start/frame.se
	@ mcopy -i fat.img boot.cfg ::/boot/limine.cfg
	@ rm BOOTX64.EFI
	@ mv fat.img build/skylight.hdd

glass: $(GLASS)
frame: $(FRAME)
image: $(IMAGE)

clean:
	@ rm -rf build
	@ make -C glass clean
	@ make -C frame clean

QEMU_MEM	= 1G
SERIAL_OUT	= stdio

QEMU_ARGS = -bios OVMF.fd \
-drive file=$(IMAGE),format=raw \
-net none \
-m $(QEMU_MEM) \
-machine q35

run:
	@ qemu-system-x86_64 $(QEMU_ARGS) $(ADD_QEMU_ARGS)

debug:
	@ qemu-system-x86_64 $(QEMU_ARGS) -S -gdb tcp::1234 -d int -no-shutdown -no-reboot $(ADD_QEMU_ARGS)

debug-screen:
	@ screen -S qemu -d -m qemu-system-x86_64 $(QEMU_ARGS) -S -gdb tcp::1234 -d int -no-shutdown -no-reboot $(ADD_QEMU_ARGS)
	@ echo "Created QEMU instance in detached screen."
	
