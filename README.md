# 🌙 Luna OS (Project AETERNA)

![Build Status](https://img.shields.io/badge/build-passing-brightgreen)
![Architecture](https://img.shields.io/badge/arch-x86%20|%20arm64%20|%20riscv-blue)
![Environment](https://img.shields.io/badge/env-freestanding%20(bare--metal)-orange)
![License](https://img.shields.io/badge/license-Proprietary-red)

> **A Tactical, AI-Driven, Bare-Metal Operating System Engineered for Digital Resilience.**

Luna OS (Project AETERNA) is a sovereign, 32-bit (x86) protected-mode operating system forged entirely from scratch. Operating outside the bounds and vulnerabilities of traditional host libraries (`libc`, `libstdc++`), Luna OS serves as a deterministic, cryptographically secure infrastructure. It is purpose-built for advanced cyber security operations, tactical forensic analysis, offline artificial intelligence, and absolute digital resilience.

---

## 🌐 The AETERNA Triad: Multi-Platform Topologies

Luna OS utilizes a modular, polyglot build system capable of scaling its interface and footprint across three distinct operational environments. All platforms share the same unified Ring 0 kernel and cryptographic ledger, varying only in their Ring 3 manifestation:

1. **Desktop Terminal (`AI_PLATFORM_DESKTOP`)** The primary command center. Features the full hardware-accelerated GUI compositor, LVGL shared-surface windowing, and heavy neural inference capabilities. Designed for deep offline data analysis, document processing, and forensic disk imaging.
   
2. **Mobile / Field Node (`AI_PLATFORM_MOBILE`)** A lightweight, touch-optimized footprint designed for rapid tactical deployment. Prioritizes the NLP intent resolver for quick command execution and utilizes the native pentest suite (`luna_netstat`, `luna_portscan`) for field-deployable security auditing and network node evaluation.

3. **IoT / Edge Sensor (`AI_PLATFORM_IOT`)** Headless, hyper-efficient, and executing in strict memory constraints. Driven almost entirely by the Explicit Rule Engine to act as an autonomous data collection agent, network monitor, or drone logic controller.

---

## 🏗️ Master Architecture Blueprint

As defined in the foundational `LUNA_OS_MASTER_ARCHITECTURE.md`, the codebase is strictly segregated to ensure architectural purity. Hardware-specific assembly is isolated from the C kernel core, ensuring future-proof cross-compilation (x86, arm64, riscv). 

The operating system is built across **Five Development Tracks**:

### Track 1: Hardware & Boot Sequence (The Genesis)
* **Bootloader (`boot.asm`):** Utilizes Multiboot2 compliance to allow GRUB to load the kernel into memory, set up the initial stack, and execute the critical jump to higher-half memory (`0xC0000000`).
* **CPU Descriptor Tables:** * `gdt.c`: Defines the Global Descriptor Table with 6 strict segments (Null, Ring 0 Code/Data, Ring 3 Code/Data, and the hardware TSS for stack switching).
  * `idt.c`: Configures the Interrupt Descriptor Table with 256 gates, routing CPU exceptions and hardware interrupts safely to the kernel.
* **Hardware Abstraction:** * `pic.c`: Remaps the 8259 Programmable Interrupt Controller to avoid protected-mode collisions.
  * `timer.c`: Configures the Programmable Interval Timer (PIT) to a 1000Hz tick, driving both the kernel scheduler and the GUI compositor's refresh rate.

### Track 2: Memory Management (The Foundation)
Luna OS violently rejects the standard C `malloc` implementation in favor of a multi-tiered, fragmentation-resistant memory hierarchy:
* **PMM (`pmm.c`):** A bitmap-driven Physical Memory Manager tracking raw RAM blocks.
* **VMM (`vmm.c`):** A Virtual Memory Manager handling paging directories, ensuring user-space applications cannot peek into kernel memory.
* **Kernel Heap (`heap.c`):** A slab allocator utilized solely for long-lived kernel structures (PCBs, VFS nodes).
* **Memory Arenas (`arena.c`):** Contiguous, monolithic memory blocks used by unpredictable subsystems (like the LunaScript parser and AI inference). Eliminates use-after-free vulnerabilities by wiping entire execution states in a single `arena_reset()` operation.

### Track 3: Process Isolation & IPC (The Gatekeeper)
* **Privilege Separation:** Applications execute in Ring 3 (User Space). To access hardware, they must trigger the `int 0x80` assembly stub.
* **Syscall Dispatch (`syscall.c`):** The Master Gate. Routes standardized calls (Read, Write, Fork, Map Surface, AI Query) to the kernel. Every syscall is validated against the DAG-Ledger's security constraints before execution.

### Track 4: The GUI Compositor & Windowing
Luna OS features a native windowing system that does not rely on X11 or Wayland.
* **Shared Surfaces (`ipc/shared_surface.c`):** Allows Ring 3 applications to write to memory buffers that the kernel seamlessly blits to the screen.
* **Compositor Engine (`compositor/window.c`, `wm_events.c`):** Manages Z-indexing, window focus, and overlapping GUI elements.
* **LVGL Bridge (`lvgl_bridge.c`, `lvgl_input.c`):** Hardware-accelerated graphics library integration for drawing modern UI elements, rendering PSF2 fonts (`font.c`), and handling mouse/cursor events.

### Track 5: The Dual-Core Artificial Intelligence Subsystem
Luna OS does not rely on cloud APIs. It runs complex intelligence offline, directly on bare metal, without a Floating Point Unit (FPU).

* **Neural Inference Engine (`neural.c`, `matrix.c`):** A custom Transformer model implementation. Utilizes Q8.8 fixed-point arithmetic to perform matrix math and multi-head attention (`attention.c`). It dynamically loads quantized GGUF weights from the disk (`model_loader.c`) and encodes text using a custom Byte-Pair tokenizer (`tokenizer.c`).
* **Explicit Rule Engine (`explicit.c`, `rule_table.c`):** A deterministic, symbolic AI that acts as the ultimate authority. It evaluates system state against a hardcoded rule table. If an action violates operational security, the Explicit Engine blocks it with 100% confidence, overriding the Neural Core.
* **World Simulation (`sim_world.c`):** An entity-component engine nested within the Explicit Core. Tracks up to 128 autonomous agents navigating via Manhattan distance physics, perfect for IoT sensor logic or offline agent simulation.
* **NLP Intent Resolver (`nlp.c`, `intent_table.c`):** Translates conversational English ("show me the log files") directly into hard shell commands ("cat /var/log/sys").

---

## 🔐 Cryptographic DAG-Ledger & Virtual File System (VFS)

The Virtual File System in Luna OS is strictly tied to forensic integrity. 
* Every file creation, modification, or deletion is cryptographically hashed.
* Hashes are appended to an internal **Directed Acyclic Graph (DAG) Ledger**.
* This ensures an immutable chain-of-custody for data collection and guarantees that a compromised Ring 3 application cannot silently alter system logs. 
* Cryptographic primitives are accelerated via pure Assembly (`luna_aes.asm`, `luna_hash.asm`), bypassing software visual timing attacks via direct silicon state transformation.

---

## 📜 LunaScript (Native Interpreted Language)

Luna OS ships with its own freestanding scripting language (`lang/`) for rapid system automation, completely bypassing the need for Python or Bash.
* **Syntax:** Python-style indentation blocks mixed with JavaScript-like dynamic semantics.
* **Engine:** Built entirely in C with a custom Lexer (`lexer.c`), Recursive Descent Parser (`parser.c`), and Abstract Syntax Tree (`ast.h`).
* **Memory Safety:** Evaluated natively in Ring 0 or Ring 3, drawing all execution memory from a strict Kernel Arena.

---

## ⚔️ The Arsenal: Tactical Tools & Application Suite

Luna OS is pre-loaded with a suite of freestanding C++ and Assembly binaries, ready for immediate tactical deployment.

### Ring 3 Security & Forensic Tools (`tools/`)
* 📡 **`luna_netstat`:** Network telemetry auditor. Queries Ring 0 sockets and assigns a DAG-Ledger evaluated "Trust Score" to active connections.
* 🎯 **`luna_portscan`:** High-speed raw SYN packet injection scanner for rapid network node evaluation.
* 💾 **`luna_imager`:** Bit-stream forensic disk acquisition tool. Bypasses the VFS to directly stream sectors from `/dev/ata0` into a forensic image file.

### GUI Application Suite (`apps/`)
* 📝 **`textedit`:** Secure, shared-surface text buffer for raw code and note editing.
* 📊 **`spreadsheet`:** Tabular data analysis and computation application.
* 📑 **`wordedit`:** Rich document processing application.
* 🖩 **`calculator`:** A bare-metal GUI application written entirely in Assembly, demonstrating extreme low-level system call interaction.

---

## ⚙️ Compilation & Deployment Sequence

### Prerequisites
Compilation requires a pristine cross-compiler toolchain to prevent host-OS standard library contamination. Do not use a standard Linux GCC installation.
* `i686-elf-gcc` and `i686-elf-g++` (Freestanding Compiler)
* `nasm` (Netwide Assembler)
* `qemu-system-i386` (Hardware Emulator)
* `grub-mkrescue` / `xorriso` (ISO Generation)

### The Master Makefile
The top-level `Makefile` is a polyglot orchestrator. It automatically locates all `.c`, `.cpp`, and `.asm` files across the repository, enforces strict freestanding flags (`-nostdlib`, `-fno-pic`, `-static`), and dynamically links the internal compiler math libraries (`libgcc`).
