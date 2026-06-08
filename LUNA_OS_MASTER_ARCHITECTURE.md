# LUNA OS — PROJECT AETERNA
## MASTER ARCHITECTURE & COMPLETE FILE REFERENCE
### Version 2.0 — Phase II Blueprint

---

## 1. COMPLETE REPOSITORY STRUCTURE

```
luna-os/
│
├── LUNA_OS_MASTER_ARCHITECTURE.md   ← This file
├── Makefile                         ← Top-level polyglot build orchestrator
├── linker.ld                        ← x86 higher-half kernel linker script
│
├── arch/                            ── ARCHITECTURE-SPECIFIC (never in kernel/)
│   ├── x86/
│   │   ├── boot.asm                 ← Multiboot2 entry, stack, higher-half jump
│   │   ├── gdt.c                    ← GDT: 7 segments (null/k-code/k-data/u-code/u-data/tss/...)
│   │   ├── gdt.h
│   │   ├── gdt_flush.asm            ← lgdt + far jump to reload CS
│   │   ├── idt.c                    ← 256 gates, int 0x80 syscall gate
│   │   ├── idt.h
│   │   ├── interrupts.asm           ← ISR stubs, IRQ stubs, syscall stub
│   │   ├── pic.c                    ← 8259 PIC remap
│   │   ├── pic.h
│   │   ├── paging.c                 ← Higher-half page directory, map_page()
│   │   ├── paging.h
│   │   ├── timer.c                  ← PIT 1000Hz → scheduler tick + lv_tick_inc
│   │   └── timer.h
│   ├── arm64/
│   │   ├── boot_arm.S               ← AArch64 exception level drop, stack
│   │   ├── mmu.c                    ← ARMv8 MMU, TTBR0/TTBR1
│   │   ├── gic.c                    ← Generic Interrupt Controller
│   │   └── timer_arm.c              ← CNTFRQ_EL0 system timer
│   └── riscv/
│       └── boot_rv.S                ← Future RISC-V entry
│
├── hal/                             ── HARDWARE ABSTRACTION LAYER (the portability contract)
│   ├── hal.h                        ← Function-pointer table: hal_timer_init, hal_irq_register...
│   ├── hal_x86.c                    ← x86 implementation of hal.h contract
│   └── hal_arm64.c                  ← ARM64 implementation of hal.h contract
│
├── kernel/                          ── ARCHITECTURE-AGNOSTIC KERNEL CORE
│   ├── main.c                       ← kmain(): parse multiboot2, init all subsystems
│   ├── pmm.c / pmm.h                ← Physical Memory Manager (bitmap over mmap)
│   ├── vmm.c / vmm.h                ← Virtual Memory Manager (map_page/unmap_page)
│   ├── heap.c / heap.h              ← Kernel slab allocator (k_malloc/k_free)
│   ├── arena.c / arena.h            ← Arena allocator (for interpreter + transient allocs)
│   ├── process.c / process.h        ← PCB, process_create(), spawn_ring3()
│   ├── scheduler.c / scheduler.h    ← Preemptive priority scheduler
│   ├── syscall.c / syscall.h        ← int 0x80 dispatch table
│   ├── vfs.c / vfs.h                ← Virtual File System (DAG-indexed)
│   ├── ledger.c / ledger.h          ← DAG-Ledger forensic chain (async pipeline)
│   ├── ledger_queue.c / ledger_queue.h ← Lock-free ring buffer for ledger events
│   ├── hash.c / hash.h              ← FNV-1a 64-bit + SHA-256 freestanding
│   ├── shell.c / shell.h            ← Tactical CLI + NLP intent router
│   ├── string.c / string.h          ← Bare-metal string primitives
│   ├── multiboot2.h                 ← Multiboot2 tag structure definitions
│   ├── spinlock.h                   ← Test-and-set spinlock (asm volatile)
│   └── cpp_support.c                ← __cxa_pure_virtual, global ctor walker
│   │
│   └── ai/                          ── LUNA AI CORE
│       ├── ai_core.h                ← Unified AI dispatch interface
│       ├── ai_core.c                ← Routes queries: Explicit Engine vs Neural Core
│       │
│       ├── explicit/                ── EXPLICIT RULE ENGINE (deterministic simulation AI)
│       │   ├── explicit.h
│       │   ├── explicit.c           ← Constraint graph evaluator
│       │   ├── sim_world.c          ← World-state simulation (RCT-style agent loop)
│       │   ├── sim_world.h
│       │   ├── rule_table.c         ← Static rule database (if-then production rules)
│       │   └── rule_table.h
│       │
│       ├── neural/                  ── NEURAL CORE (freestanding matrix math LLM inference)
│       │   ├── neural.h
│       │   ├── neural.c             ← Inference dispatcher
│       │   ├── matrix.c             ← Freestanding BLAS-like matrix ops (no libm)
│       │   ├── matrix.h
│       │   ├── tokenizer.c          ← BPE tokenizer (freestanding, no stdlib)
│       │   ├── tokenizer.h
│       │   ├── attention.c          ← Scaled dot-product attention (transformer)
│       │   ├── attention.h
│       │   └── model_loader.c       ← Load GGUF quantized weights from VFS
│       │
│       └── nlp/                     ── NATURAL LANGUAGE INTENT PARSER (shell feature)
│           ├── nlp.h
│           ├── nlp.c                ← Intent classifier: routes English → shell commands
│           ├── intent_table.c       ← Static intent→command mapping table
│           └── intent_table.h
│
├── ipc/                             ── INTER-PROCESS COMMUNICATION
│   ├── shared_surface.c / shared_surface.h  ← Zero-copy pixel surface for compositor
│   ├── message_queue.c / message_queue.h    ← Typed message passing (Ring 3 ↔ Ring 3)
│   └── lmp_frame.h                          ← Luna Mesh Protocol frame (cross-device)
│
├── lang/                            ── LUNA SCRIPTING LANGUAGE INTERPRETER
│   ├── token.h                      ← Token enum (all 80+ token types)
│   ├── lexer.c / lexer.h            ← Tokenizer + indent stack (Python-style blocks)
│   ├── ast.h                        ← AST node tagged union (arena-allocated)
│   ├── parser.c / parser.h          ← Recursive descent parser
│   ├── value.h                      ← Value type (int/float/str/fn/list/null)
│   ├── env.c / env.h                ← Symbol table (hash map over arena)
│   ├── eval.c / eval.h              ← Tree-walk evaluator
│   └── stdlib.c / stdlib.h          ← Built-ins: print, hash, ledger_read, net_send
│
├── drivers/
│   ├── gfx/
│   │   ├── framebuffer.c / framebuffer.h  ← LFB: fb_init, fb_put_pixel, fb_blit
│   │   ├── font.c / font.h                ← PSF2 bitmap font renderer
│   │   ├── cursor.c / cursor.h            ← Hardware cursor from mouse_x/mouse_y
│   │   ├── lvgl_bridge.c / lvgl_bridge.h  ← LVGL ↔ LFB flush callback + tick
│   │   └── lvgl_input.c / lvgl_input.h    ← LVGL indev wired to PS/2
│   ├── input/
│   │   ├── keyboard.c / keyboard.h        ← PS/2 keyboard (scancode → keycode)
│   │   └── mouse.c / mouse.h              ← PS/2 mouse (IRQ12, 3-byte packet)
│   ├── net/
│   │   ├── nic.c / nic.h                  ← Raw NIC TX/RX ring buffer (RTL8139/e1000)
│   │   ├── lmp.c / lmp.h                  ← Luna Mesh Protocol transport (cross-device)
│   │   └── raw_eth.c / raw_eth.h          ← Raw Ethernet frame inject/capture
│   ├── storage/
│   │   ├── ata.c / ata.h                  ← ATA PIO disk read/write
│   │   └── disk_image.c / disk_image.h    ← Forensic bit-stream imager (no timestamp modify)
│   └── serial/
│       ├── uart.c / uart.h                ← UART 16550 (debug output + IoT sync)
│       └── can.c / can.h                  ← CAN bus driver (IoT/vehicle/drone)
│
├── compositor/                      ── RING 3 WINDOW COMPOSITOR
│   ├── compositor.c / compositor.h   ← Main compositor loop, z-order, damage regions
│   ├── window.c / window.h           ← Window struct, create/destroy/resize
│   └── wm_events.c / wm_events.h     ← Input routing: PS/2 events → focused window
│
├── tools/                           ── BUILT-IN SYSTEM TOOLS (Ring 3, C++/Rust/ASM)
│   ├── nettools/
│   │   ├── luna_netstat.cpp         ← Network state viewer (C++)
│   │   ├── luna_ping.cpp            ← ICMP echo (raw socket via syscall)
│   │   ├── luna_tcpdump.rs          ← Packet capture (Rust no_std)
│   │   └── luna_arp.asm             ← ARP table reader (Assembly)
│   ├── pentest/
│   │   ├── luna_portscan.cpp        ← TCP SYN scanner (C++)
│   │   ├── luna_exploit.cpp         ← Payload framework scaffold (C++)
│   │   └── luna_wifi_probe.rs       ← 802.11 probe frame injector (Rust)
│   ├── crypto/
│   │   ├── luna_aes.asm             ← AES-256-GCM (pure x86 Assembly + AES-NI)
│   │   ├── luna_chacha.c            ← ChaCha20-Poly1305 (freestanding C)
│   │   ├── luna_keygen.cpp          ← Key generation + management (C++)
│   │   └── luna_pqc.rs              ← Post-quantum primitives: CRYSTALS-Kyber (Rust)
│   ├── forensics/
│   │   ├── luna_imager.cpp          ← Forensic disk imager (C++)
│   │   ├── luna_ledger_dump.cpp     ← DAG-Ledger reader/exporter (C++)
│   │   └── luna_recover.rs          ← Deleted file recovery (Rust)
│   ├── recon/
│   │   ├── luna_osint.cpp           ← OSINT aggregator (C++)
│   │   ├── luna_whois.cpp           ← WHOIS engine (C++)
│   │   └── luna_traceroute.asm      ← Traceroute (TTL-decrement, Assembly)
│   ├── autopsy/
│   │   ├── luna_autopsy.cpp         ← Disk artifact analyzer (C++)
│   │   └── luna_timeline.cpp        ← Event timeline builder from ledger (C++)
│   └── filehasher/
│       ├── luna_hash.asm            ← High-speed file hasher (SHA-256, Assembly)
│       └── luna_verify.cpp          ← Hash verification + report (C++)
│
├── apps/                            ── BUILT-IN APPLICATIONS (Ring 3, C++/Rust)
│   ├── textedit/
│   │   └── main.cpp                 ← Text editor (C++, LVGL UI)
│   ├── wordedit/
│   │   └── main.cpp                 ← Word processor with basic formatting (C++)
│   ├── spreadsheet/
│   │   └── main.cpp                 ← Spreadsheet engine + LVGL grid (C++)
│   ├── presentation/
│   │   └── main.cpp                 ← Slide engine (C++)
│   ├── calculator/
│   │   ├── main.asm                 ← Scientific calculator UI (Assembly — pure speed)
│   │   └── calc_engine.c            ← Arithmetic engine with freestanding float
│   ├── camera/
│   │   └── main.cpp                 ← Camera capture (UVC/V4L-style driver bridge, C++)
│   ├── comms/
│   │   ├── main.cpp                 ← Secure messaging app (C++)
│   │   └── crypto_layer.rs          ← E2E encryption layer (Rust, ChaCha20)
│   ├── browser/
│   │   └── main.cpp                 ← HTTP client + minimal HTML renderer (C++)
│   ├── maps/
│   │   └── main.cpp                 ← GPS/tile map renderer (C++)
│   ├── ai_sim/
│   │   └── main.cpp                 ← AI simulation sandbox (bridges ai_core.h, C++)
│   └── ftp/
│       ├── main.cpp                 ← FTP/HTTP client+server (C++)
│       └── ftp_engine.rs            ← Protocol engine (Rust)
│
├── rust/                            ── RUST NO_STD CRATES
│   ├── luna_hal/
│   │   ├── Cargo.toml
│   │   └── src/lib.rs               ← HAL bindings (no_std, unsafe FFI to C)
│   ├── luna_net/
│   │   ├── Cargo.toml
│   │   └── src/lib.rs               ← Network driver (no_std)
│   └── luna_crypto/
│       ├── Cargo.toml
│       └── src/lib.rs               ← Crypto suite (no_std)
│
├── include/                         ── SHARED CROSS-MODULE HEADERS
│   ├── types.h                      ← uint8_t, uint32_t, uintptr_t, bool (no stdint)
│   ├── math_prim.h                  ← __udivsi3, __umodsi3 declarations
│   └── panic.h                      ← kernel_panic() declaration
│
├── boot/                            ← Legacy VGA fallback + boot helpers
│   └── vga.c / vga.h               ← VGA text mode (0xB8000) — Phase I legacy
│
├── scripts/
│   ├── run_qemu.sh                  ← QEMU launch script (x86, -cdrom lunaos.iso)
│   ├── run_qemu_arm.sh              ← QEMU ARM64 launch
│   └── gdb_debug.sh                 ← GDB remote debug via QEMU -s -S
│
└── iso/
    └── boot/
        └── grub/
            └── grub.cfg             ← GRUB2 menu config
```

---

## 2. LANGUAGE ASSIGNMENT RATIONALE

| Layer | Language | Reason |
|---|---|---|
| Boot, context switch, ISR stubs | NASM/GAS Assembly | Direct hardware control, no C overhead |
| Kernel Ring 0 core | Freestanding C | Deterministic, no runtime, max portability |
| HAL implementations | Freestanding C | Arch-specific but C-interoperable |
| AI Explicit Engine | Freestanding C | Must run in Ring 0, pure logic |
| AI Neural Core | Freestanding C (matrix) | No-stdlib matrix math |
| NLP Intent Layer | Freestanding C | Shell-integrated, Ring 0 |
| Scripting Language (Luna lang) | Freestanding C | Arena-allocated, no malloc |
| Compositor / WM | C++ (freestanding) | OOP window tree, LVGL API |
| Security Tools | C++ primary, Rust where memory-safety critical | Pentest = C++; crypto = Rust |
| Calculator UI | Assembly | Pure computational speed, minimal LOC |
| File Hasher | Assembly (SHA-256) | AES-NI / SHA-NI hardware instructions |
| AES-256-GCM | Assembly | AES-NI intrinsics at instruction level |
| Network Driver | Rust (no_std) | Memory-safe packet handling |
| Crypto Suite | Rust (no_std) | Post-quantum: safe Rust abstractions |
| Apps (editors, browser, etc.) | C++ | Complex UI, OOP data models |
| E2E Comms crypto | Rust | Zero memory vulnerabilities in crypto path |

---

## 3. AI SYSTEM ARCHITECTURE

### 3.1 Dual-Engine Design

```
User Input (shell / GUI)
        │
        ▼
┌───────────────────┐
│   AI CORE         │  ai/ai_core.c
│   Query Router    │
└────────┬──────────┘
         │
    ┌────┴────┐
    │         │
    ▼         ▼
┌──────────┐  ┌────────────────┐
│ EXPLICIT │  │  NEURAL CORE   │
│ ENGINE   │  │  (LLM Infer.)  │
│          │  │                │
│ Rule DB  │  │ Transformer    │
│ Sim World│  │ Attention      │
│ Agents   │  │ Tokenizer      │
│ Constraints│ │ Model Weights │
└────┬─────┘  └───────┬────────┘
     │                │
     └────────┬────────┘
              ▼
     ┌─────────────────┐
     │  FUSION LAYER   │  Weighted confidence merge
     │  (ai_core.c)    │  Explicit = ground truth
     └────────┬────────┘  Neural = language + context
              ▼
       Final Response
```

### 3.2 NLP Intent Layer (separate from AI — shell feature)

```
English CLI Input: "show me all files modified today"
        │
        ▼
┌──────────────────────┐
│  nlp/nlp.c           │
│  Intent Classifier   │
│  (no LLM — static    │
│   pattern matching + │
│   intent_table.c)    │
└──────────┬───────────┘
           │
     Intent: FILE_LIST + FILTER_DATE_TODAY
           │
           ▼
  shell command: ls --modified=today
           │
           ▼
    vfs.c executes
```

### 3.3 Platform AI Deployment

| Platform | Explicit Engine | Neural Core | NLP |
|---|---|---|---|
| Luna Desktop (x86_64) | Full | Full (float32 weights) | Full |
| Luna Mobile (ARMv8-A) | Full | Quantized INT8 | Full |
| Luna IoT (drone/vehicle) | Deprecated (rule subset only) | None | None |

---

## 4. PHASE II BUILD ORDER (file-by-file dependency chain)

### Track 0 — Foundation Refactor (do first, everything depends on this)
1. `include/types.h` — define uint8/16/32/64, uintptr_t, bool, NULL
2. `include/math_prim.h` — __udivsi3, __umodsi3 declarations
3. `include/panic.h` — kernel_panic()
4. `Makefile` — polyglot build (C + C++ + ASM + Rust, ARCH= switch)
5. `linker.ld` — higher-half kernel at 0xC0100000

### Track 1 — LFB (critical path)
6. `arch/x86/boot.asm` — multiboot2 header + framebuffer tag
7. `kernel/multiboot2.h` — tag structs
8. `kernel/main.c` — parse multiboot2, extract fb_addr/pitch/bpp
9. `drivers/gfx/framebuffer.c` — fb_init, fb_put_pixel, fb_fill_rect, fb_blit
10. `drivers/gfx/font.c` — PSF2 font renderer

### Track 2 — Memory Management (required before interpreter + GUI)
11. `kernel/pmm.c` — bitmap physical allocator over mmap
12. `kernel/vmm.c` — map_page / unmap_page
13. `kernel/heap.c` — slab allocator (k_malloc / k_free)
14. `kernel/arena.c` — arena allocator (for interpreter)

### Track 3 — Process + Syscall (required before Ring 3)
15. `kernel/process.c` — PCB, spawn_ring3()
16. `arch/x86/idt.c` — add int 0x80 gate
17. `kernel/syscall.c` — syscall dispatch table

### Track 4 — Compositor
18. `ipc/shared_surface.c`
19. `compositor/compositor.c`
20. `compositor/window.c`
21. `drivers/gfx/cursor.c`
22. `compositor/wm_events.c`
23. `drivers/gfx/lvgl_bridge.c`
24. `drivers/gfx/lvgl_input.c`

### Track 5 — AI Core
25. `kernel/ai/nlp/intent_table.c` — static intent map
26. `kernel/ai/nlp/nlp.c` — intent classifier
27. `kernel/ai/explicit/rule_table.c`
28. `kernel/ai/explicit/sim_world.c`
29. `kernel/ai/explicit/explicit.c`
30. `kernel/ai/neural/matrix.c`
31. `kernel/ai/neural/tokenizer.c`
32. `kernel/ai/neural/attention.c`
33. `kernel/ai/neural/neural.c`
34. `kernel/ai/ai_core.c`

### Track 6 — Luna Script Interpreter
35. `lang/token.h`
36. `lang/lexer.c`
37. `lang/ast.h`
38. `lang/parser.c`
39. `lang/value.h`
40. `lang/env.c`
41. `lang/eval.c`
42. `lang/stdlib.c`

### Track 7 — Tools + Apps (parallel, after Tracks 1-4)
43+ See tools/ and apps/ directories — each is a standalone Ring 3 binary
