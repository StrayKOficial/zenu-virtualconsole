# RISC-V Toolchain Path (User should adjust this)
PREFIX = riscv64-unknown-elf-
CC = $(PREFIX)g++
AS = $(PREFIX)as
LD = $(PREFIX)ld
OBJCOPY = $(PREFIX)objcopy

# Project settings
ROM_NAME = tetris
SRC_DIR = src
INC_DIR = ../../sdk/include
SDK_SRC = ../../sdk/src/crt0.s
LINKER_SCRIPT = ../../sdk/zenu.ld

# Compiler flags
CFLAGS = -march=rv32im -mabi=ilp32 -ffreestanding -nostdlib -I$(INC_DIR) -O2

# Build targets
all: $(ROM_NAME).boc

$(ROM_NAME).elf: $(SRC_DIR)/main.cpp $(SDK_SRC)
	$(CC) $(CFLAGS) -T$(LINKER_SCRIPT) $(SDK_SRC) $(SRC_DIR)/main.cpp -o $(ROM_NAME).elf

rom.bin: $(ROM_NAME).elf
	$(OBJCOPY) -O binary $(ROM_NAME).elf rom.bin

$(ROM_NAME).boc: rom.bin
	mkdir -p $(ROM_NAME).boc
	cp rom.bin $(ROM_NAME).boc/
	echo '{"name": "Zenu Tetris", "version": "0.1.0"}' > $(ROM_NAME).boc/manifest.json

clean:
	rm -f *.elf *.bin
	rm -rf $(ROM_NAME).boc
