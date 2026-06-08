# =============================================================================
# Luna OS — Project AETERNA
# Master Polyglot Makefile
# Supports: ARCH=x86 (default) | ARCH=arm64 | ARCH=riscv
# Languages: C (freestanding), C++ (freestanding), NASM, Rust (no_std)
# =============================================================================

ARCH ?= x86
BUILD_DIR := build/$(ARCH)
ISO_DIR   := iso
TARGET    := lunaos.iso

# ── Toolchain Selection ──────────────────────────────────────────────────────
ifeq ($(ARCH), x86)
    CC       := i686-elf-gcc
    CXX      := i686-elf-g++
    LD       := i686-elf-ld
    AS       := nasm
    ASFLAGS  := -f elf32
    ARCH_DIR := arch/x86
    CFLAGS_ARCH := -DARCH_X86 -m32

else ifeq ($(ARCH), arm64)
    CC       := aarch64-none-elf-gcc
    CXX      := aarch64-none-elf-g++
    LD       := aarch64-none-elf-ld
    AS       := aarch64-none-elf-as
    ASFLAGS  :=
    ARCH_DIR := arch/arm64
    CFLAGS_ARCH := -DARCH_ARM64 -march=armv8-a

else ifeq ($(ARCH), riscv)
    CC       := riscv64-unknown-elf-gcc
    CXX      := riscv64-unknown-elf-g++
    LD       := riscv64-unknown-elf-ld
    AS       := riscv64-unknown-elf-as
    ASFLAGS  :=
    ARCH_DIR := arch/riscv
    CFLAGS_ARCH := -DARCH_RISCV -march=rv64imac -mabi=lp64
endif

# ── Rust Toolchain ───────────────────────────────────────────────────────────
CARGO     := cargo
RUST_TARGET_X86   := i686-unknown-none
RUST_TARGET_ARM64 := aarch64-unknown-none
ifeq ($(ARCH), x86)
    RUST_TARGET := $(RUST_TARGET_X86)
else
    RUST_TARGET := $(RUST_TARGET_ARM64)
endif

# ── Common C Flags ───────────────────────────────────────────────────────────
CFLAGS := \
    -ffreestanding \
    -nostdlib      \
    -nostdinc      \
    -fno-stack-protector \
    -fno-builtin   \
    -O2            \
    -Wall          \
    -Wextra        \
    -Iinclude      \
    -Ikernel       \
    $(CFLAGS_ARCH)

# ── C++ Flags (no RTTI, no exceptions — mandatory for bare-metal) ─────────────
CXXFLAGS := $(CFLAGS) \
    -fno-rtti       \
    -fno-exceptions \
    -fno-use-cxa-atexit \
    -std=c++17

# ── Linker Flags ─────────────────────────────────────────────────────────────
LDFLAGS := -T linker.ld -nostdlib

# ── Source Discovery ─────────────────────────────────────────────────────────
# Arch-specific sources
ARCH_ASM_SRCS := $(wildcard $(ARCH_DIR)/*.asm)
ARCH_C_SRCS   := $(wildcard $(ARCH_DIR)/*.c)

# HAL
HAL_SRCS := hal/hal_$(ARCH).c

# Kernel core (C only — no C++ in Ring 0 core)
KERNEL_C_SRCS := \
    kernel/main.c      \
    kernel/pmm.c       \
    kernel/vmm.c       \
    kernel/heap.c      \
    kernel/arena.c     \
    kernel/process.c   \
    kernel/scheduler.c \
    kernel/syscall.c   \
    kernel/vfs.c       \
    kernel/ledger.c    \
    kernel/ledger_queue.c \
    kernel/hash.c      \
    kernel/shell.c     \
    kernel/string.c    \
    kernel/cpp_support.c \
    kernel/ai/ai_core.c \
    kernel/ai/explicit/explicit.c \
    kernel/ai/explicit/rule_table.c \
    kernel/ai/explicit/sim_world.c \
    kernel/ai/neural/neural.c \
    kernel/ai/neural/matrix.c \
    kernel/ai/neural/tokenizer.c \
    kernel/ai/neural/attention.c \
    kernel/ai/neural/model_loader.c \
    kernel/ai/nlp/nlp.c \
    kernel/ai/nlp/intent_table.c

# Lang interpreter
LANG_SRCS := \
    lang/lexer.c  \
    lang/parser.c \
    lang/env.c    \
    lang/eval.c   \
    lang/stdlib.c

# Drivers
DRIVER_C_SRCS := \
    drivers/gfx/framebuffer.c \
    drivers/gfx/font.c        \
    drivers/gfx/cursor.c      \
    drivers/gfx/lvgl_bridge.c \
    drivers/gfx/lvgl_input.c  \
    drivers/input/keyboard.c  \
    drivers/input/mouse.c     \
    drivers/net/nic.c         \
    drivers/net/lmp.c         \
    drivers/net/raw_eth.c     \
    drivers/storage/ata.c     \
    drivers/storage/disk_image.c \
    drivers/serial/uart.c     \
    drivers/serial/can.c

# IPC
IPC_SRCS := \
    ipc/shared_surface.c \
    ipc/message_queue.c

# Compositor (C++)
COMPOSITOR_CXX_SRCS := \
    compositor/compositor.cpp \
    compositor/window.cpp     \
    compositor/wm_events.cpp

# ── Object Lists ─────────────────────────────────────────────────────────────
ARCH_ASM_OBJS    := $(patsubst $(ARCH_DIR)/%.asm, $(BUILD_DIR)/arch/%.o, $(ARCH_ASM_SRCS))
ARCH_C_OBJS      := $(patsubst $(ARCH_DIR)/%.c,   $(BUILD_DIR)/arch/%.o, $(ARCH_C_SRCS))
HAL_OBJS         := $(patsubst %.c, $(BUILD_DIR)/%.o, $(HAL_SRCS))
KERNEL_OBJS      := $(patsubst %.c, $(BUILD_DIR)/%.o, $(KERNEL_C_SRCS))
LANG_OBJS        := $(patsubst %.c, $(BUILD_DIR)/%.o, $(LANG_SRCS))
DRIVER_OBJS      := $(patsubst %.c, $(BUILD_DIR)/%.o, $(DRIVER_C_SRCS))
IPC_OBJS         := $(patsubst %.c, $(BUILD_DIR)/%.o, $(IPC_SRCS))
COMPOSITOR_OBJS  := $(patsubst %.cpp, $(BUILD_DIR)/%.o, $(COMPOSITOR_CXX_SRCS))

ALL_OBJS := \
    $(ARCH_ASM_OBJS)   \
    $(ARCH_C_OBJS)     \
    $(HAL_OBJS)        \
    $(KERNEL_OBJS)     \
    $(LANG_OBJS)       \
    $(DRIVER_OBJS)     \
    $(IPC_OBJS)        \
    $(COMPOSITOR_OBJS)

# ── Main Targets ─────────────────────────────────────────────────────────────
.PHONY: all clean iso run run-debug rust-crates tools apps

all: iso

iso: $(BUILD_DIR)/lunaos.elf rust-crates
	@mkdir -p $(ISO_DIR)/boot/grub
	@cp $(BUILD_DIR)/lunaos.elf $(ISO_DIR)/boot/lunaos.elf
	@grub-mkrescue -o $(TARGET) $(ISO_DIR) 2>/dev/null
	@echo "[LUNA] ISO built: $(TARGET)"

# ── Kernel ELF ───────────────────────────────────────────────────────────────
$(BUILD_DIR)/lunaos.elf: $(ALL_OBJS)
	@mkdir -p $(dir $@)
	@$(CC) $(LDFLAGS) -o $@ $^
	@echo "[LD]  $@"

# ── Assembly Sources ──────────────────────────────────────────────────────────
$(BUILD_DIR)/arch/%.o: $(ARCH_DIR)/%.asm
	@mkdir -p $(dir $@)
	@$(AS) $(ASFLAGS) -o $@ $<
	@echo "[ASM] $<"

# ── C Sources ─────────────────────────────────────────────────────────────────
$(BUILD_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	@$(CC) $(CFLAGS) -c -o $@ $<
	@echo "[CC]  $<"

# ── C++ Sources ───────────────────────────────────────────────────────────────
$(BUILD_DIR)/%.o: %.cpp
	@mkdir -p $(dir $@)
	@$(CXX) $(CXXFLAGS) -c -o $@ $<
	@echo "[CXX] $<"

# ── Rust Crates ───────────────────────────────────────────────────────────────
rust-crates:
	@echo "[RUST] Building luna_hal..."
	@cd rust/luna_hal   && $(CARGO) build --release --target $(RUST_TARGET) 2>/dev/null
	@echo "[RUST] Building luna_net..."
	@cd rust/luna_net   && $(CARGO) build --release --target $(RUST_TARGET) 2>/dev/null
	@echo "[RUST] Building luna_crypto..."
	@cd rust/luna_crypto && $(CARGO) build --release --target $(RUST_TARGET) 2>/dev/null

# ── Tools (Ring 3 binaries) ───────────────────────────────────────────────────
tools:
	@echo "[TOOLS] Building security toolset..."
	$(CXX) $(CXXFLAGS) -o $(BUILD_DIR)/luna_netstat  tools/nettools/luna_netstat.cpp
	$(CXX) $(CXXFLAGS) -o $(BUILD_DIR)/luna_portscan tools/pentest/luna_portscan.cpp
	$(CXX) $(CXXFLAGS) -o $(BUILD_DIR)/luna_imager   tools/forensics/luna_imager.cpp
	$(AS)  $(ASFLAGS)  -o $(BUILD_DIR)/luna_aes.o    tools/crypto/luna_aes.asm
	$(AS)  $(ASFLAGS)  -o $(BUILD_DIR)/luna_hash.o   tools/filehasher/luna_hash.asm

# ── Apps (Ring 3 binaries) ────────────────────────────────────────────────────
apps:
	@echo "[APPS] Building application suite..."
	$(CXX) $(CXXFLAGS) -o $(BUILD_DIR)/textedit      apps/textedit/main.cpp
	$(CXX) $(CXXFLAGS) -o $(BUILD_DIR)/wordedit      apps/wordedit/main.cpp
	$(CXX) $(CXXFLAGS) -o $(BUILD_DIR)/spreadsheet   apps/spreadsheet/main.cpp
	$(AS)  $(ASFLAGS)  -o $(BUILD_DIR)/calculator.o  apps/calculator/main.asm

# ── Run in QEMU ───────────────────────────────────────────────────────────────
run: iso
	@qemu-system-i386 \
		-cdrom $(TARGET) \
		-m 256M \
		-vga std \
		-serial stdio \
		-no-reboot

run-debug: iso
	@qemu-system-i386 \
		-cdrom $(TARGET) \
		-m 256M \
		-vga std \
		-serial stdio \
		-s -S &
	@sleep 1
	@gdb $(BUILD_DIR)/lunaos.elf \
		-ex "target remote :1234" \
		-ex "symbol-file $(BUILD_DIR)/lunaos.elf"

# ── Clean ─────────────────────────────────────────────────────────────────────
clean:
	@rm -rf build/ $(TARGET)
	@echo "[LUNA] Clean complete"
