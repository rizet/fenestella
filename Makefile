OUTPUT	= bin
KERNEL	= $(OUTPUT)/kernel.sys

IMAGE	= $(OUTPUT)/skylight.hdd
IMGDIR	= $(OUTPUT)/image
IMGTMP 	= fat.img
FATTEN	= fatten.sh

LIMINE-LINK		= https://github.com/limine-bootloader/limine/raw/v7.x-binary/BOOTX64.EFI
LIMINE-FILE		= limine.efi

APERTURE-OUT	= $(OUTPUT)/aperture.se

.DEFAULT-GOAL	= image
.PHONY			= clean rmtmpbin run debug debug-screen

KERNEL-MK	= glass.mk

$(IMAGE): rmtmpbin kernel frames
	@ echo "generating system image..."
## 	download the latest Limine EFI file
	@ echo "	downloading bootloader image..."
	@ wget $(LIMINE-LINK) -O $(LIMINE-FILE) --quiet
## 	organize the structure of the image
	@ echo "	organizing file contents..."
	@ mkdir $(IMGDIR)
	@ mkdir $(IMGDIR)/efi
	@ mkdir $(IMGDIR)/efi/boot
	@ mkdir $(IMGDIR)/sys
	@ mkdir $(IMGDIR)/sys/start
	@ mkdir $(IMGDIR)/sys/share
	@ cp $(LIMINE-FILE) $(IMGDIR)/efi/boot/BOOTX64.EFI
	@ cp $(KERNEL) $(IMGDIR)/sys/start/kernel.sys
	@ cp $(APERTURE-OUT) $(IMGDIR)/sys/start/aperture.se
	@ cp boot.cfg $(IMGDIR)/limine.cfg
## 	compile the file contents into an image
	@ echo "	formatting final image..."
	@ dd if=/dev/zero of="$(IMGTMP)" bs=1M count="1024"
	@ mformat -i "$(IMGTMP)" -F -v "FAT32_IMAGE" ::
	@ mcopy -i "$(IMGTMP)" -s -v "$(IMGDIR)"/* ::/ > /dev/null 2>&1
## 	clean up and finalize
	@ echo "	cleaning up..."
	@ rm -rfd $(IMGDIR)
	@ mv $(IMGTMP) $(IMAGE)
	@ rm -f $(LIMINE-FILE)
	@ echo "done."



# only removes copied binaries and image
# done to avoid recompiling everything
# done to force for dependencies to be checked
rmtmpbin:
	@ rm -rf $(OUTPUT)

KERNEL_DIR	= kernel
$(KERNEL): rmtmpbin
	@ make -C $(KERNEL_DIR) -j12 all
	@ mkdir -p $(OUTPUT)
	@ cp $(KERNEL_DIR)/$(KERNEL) $(KERNEL)

APPS_DIR	= apps
FRAMES_DIR	= $(APPS_DIR)/frames

APERTURE_DIR	= $(FRAMES_DIR)/aperture
$(APERTURE-OUT): rmtmpbin
	@ make -C $(APERTURE_DIR) -j12 all
	@ mkdir -p $(OUTPUT)
	@ cp $(APERTURE_DIR)/$(APERTURE-OUT) $(APERTURE-OUT)

kernel: $(KERNEL)

aperture: $(APERTURE-OUT)
frames: aperture

image: $(IMAGE)


clean:
	@ rm -rf $(OUTPUT)
	@ make -C $(KERNEL_DIR) clean
	@ make -C $(APERTURE_DIR) clean

QEMU_MEM	= 1G
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
	