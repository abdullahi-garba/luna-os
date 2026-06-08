/* kernel/main.c — Luna OS kmain()
 * Central kernel entry point. Called from boot.asm after paging is live.
 * Initializes all Ring 0 subsystems in strict dependency order.
 */

#include "multiboot2.h"
#include "pmm.h"
#include "vmm.h"
#include "heap.h"
#include "arena.h"
#include "string.h"
#include "scheduler.h"
#include "process.h"
#include "syscall.h"
#include "vfs.h"
#include "ledger.h"
#include "shell.h"
#include "ai/ai_core.h"
#include "../include/types.h"
#include "../include/panic.h"
#include "../hal/hal.h"
#include "../drivers/gfx/framebuffer.h"
#include "../drivers/gfx/font.h"
#include "../drivers/input/keyboard.h"
#include "../drivers/input/mouse.h"
#include "../drivers/serial/uart.h"
#include "../arch/x86/gdt.h"
#include "../arch/x86/idt.h"
#include "../arch/x86/pic.h"
#include "../arch/x86/paging.h"
#include "../arch/x86/timer.h"

#define KERNEL_VIRT_BASE 0xC0000000U

/* Framebuffer info — populated by parse_multiboot2() */
static uint32_t fb_addr   = 0;
static uint32_t fb_pitch  = 0;
static uint32_t fb_width  = 0;
static uint32_t fb_height = 0;
static uint8_t  fb_bpp    = 0;

/* Memory map for PMM */
#define MAX_MMAP_ENTRIES 64
static MB2_MmapEntry mmap_entries[MAX_MMAP_ENTRIES];
static uint32_t      mmap_count = 0;
static uint64_t      mem_total  = 0;

/* ─────────────────────────────────────────────────────────────────────────── */
static void parse_multiboot2(uint32_t mb2_phys_addr) {
    /* Adjust physical pointer to virtual — kernel is higher-half mapped */
    MB2_Info* info = (MB2_Info*)(mb2_phys_addr + KERNEL_VIRT_BASE);
    MB2_Tag*  tag  = MB2_FIRST_TAG(info);

    while (tag->type != MB2_TAG_END) {
        switch (tag->type) {

        case MB2_TAG_FRAMEBUFFER: {
            MB2_TagFramebuffer* fb = (MB2_TagFramebuffer*)tag;
            fb_addr   = (uint32_t)fb->framebuffer_addr;
            fb_pitch  = fb->framebuffer_pitch;
            fb_width  = fb->framebuffer_width;
            fb_height = fb->framebuffer_height;
            fb_bpp    = fb->framebuffer_bpp;
            break;
        }

        case MB2_TAG_MMAP: {
            MB2_TagMmap* mmap = (MB2_TagMmap*)tag;
            uint32_t num = (mmap->size - sizeof(MB2_TagMmap)) / mmap->entry_size;
            MB2_MmapEntry* entry = (MB2_MmapEntry*)((uint8_t*)mmap + sizeof(MB2_TagMmap));

            for (uint32_t i = 0; i < num && mmap_count < MAX_MMAP_ENTRIES; i++) {
                mmap_entries[mmap_count++] = entry[i];
                if (entry[i].type == MB2_MMAP_AVAILABLE) {
                    mem_total += entry[i].length;
                }
            }
            break;
        }

        default:
            break;
        }

        tag = MB2_NEXT_TAG(tag);
    }
}

/* ─────────────────────────────────────────────────────────────────────────── */
static void boot_banner(void) {
    font_set_fg(0x00E5FF); /* Luna cyan */
    font_set_bg(0x000000);
    font_print(4, 4,
        "  _                         ___  ____  \n"
        " | |   _   _ _ __   __ _  / _ \\/ ___| \n"
        " | |  | | | | '_ \\ / _` || | | \\___ \\ \n"
        " | |__| |_| | | | | (_| || |_| |___) |\n"
        " |_____\\__,_|_| |_|\\__,_| \\___/|____/ \n"
    );
    font_set_fg(0xFFFFFF);
    font_print(4, 80, "Project AETERNA — Ring 0 Kernel Active");
    font_set_fg(0x888888);
    font_print(4, 96, "Build: " __DATE__ " " __TIME__);
}

/* ─────────────────────────────────────────────────────────────────────────── */
void kmain(uint32_t mb2_magic, uint32_t mb2_phys_addr) {

    /* ── 1. Serial debug output (works before everything else) ─────────── */
    uart_init();
    uart_puts("[LUNA] Boot started\n");

    /* ── 2. Validate multiboot2 magic ────────────────────────────────── */
    if (mb2_magic != MULTIBOOT2_MAGIC) {
        uart_puts("[PANIC] Not loaded by a Multiboot2 bootloader!\n");
        kernel_panic("Invalid multiboot2 magic");
    }

    /* ── 3. Parse multiboot2 tags ────────────────────────────────────── */
    parse_multiboot2(mb2_phys_addr);
    uart_puts("[MB2] Parsed: framebuffer + mmap\n");

    /* ── 4. GDT: 7 segments (null, k-code, k-data, u-code, u-data, TSS) */
    gdt_install();
    uart_puts("[GDT] Installed\n");

    /* ── 5. IDT: 256 gates + PIC remap + int 0x80 syscall gate ───────── */
    idt_install();
    pic_remap(0x20, 0x28);
    uart_puts("[IDT] Installed, PIC remapped\n");

    /* ── 6. Physical Memory Manager ──────────────────────────────────── */
    pmm_init(mmap_entries, mmap_count);
    uart_puts("[PMM] Initialized — ");
    /* log mem_total KB */

    /* ── 7. Virtual Memory Manager (higher-half kernel pages) ────────── */
    vmm_init();
    uart_puts("[VMM] Initialized\n");

    /* ── 8. Kernel heap (slab allocator) ────────────────────────────── */
    heap_init();
    uart_puts("[HEAP] Slab allocator online\n");

    /* ── 9. Arena allocator (for interpreter + AI) ───────────────────── */
    arena_global_init();
    uart_puts("[ARENA] Initialized\n");

    /* ── 10. Linear Framebuffer ──────────────────────────────────────── */
    if (fb_addr && fb_bpp == 32) {
        fb_init(fb_addr, fb_pitch, fb_width, fb_height, fb_bpp);
        font_init();
        uart_puts("[LFB] Framebuffer online\n");
        boot_banner();
    } else {
        uart_puts("[LFB] No 32bpp framebuffer — text mode fallback\n");
    }

    /* ── 11. PIT Timer (1000Hz) ──────────────────────────────────────── */
    timer_init(1000);
    uart_puts("[PIT] 1000Hz timer started\n");

    /* ── 12. Input Drivers ───────────────────────────────────────────── */
    keyboard_install();
    mouse_install();
    uart_puts("[INPUT] PS/2 keyboard + mouse installed\n");

    /* ── 13. Virtual File System ─────────────────────────────────────── */
    vfs_init();
    uart_puts("[VFS] Initialized\n");

    /* ── 14. DAG-Ledger (forensic chain) ────────────────────────────── */
    ledger_init();
    uart_puts("[LEDGER] DAG-Ledger online\n");

    /* ── 15. Process subsystem + syscall table ───────────────────────── */
    process_init();
    syscall_init();
    uart_puts("[PROC] Process subsystem + syscalls ready\n");

    /* ── 16. AI Core (Explicit Engine + Neural Core + NLP) ───────────── */
    ai_core_init();
    uart_puts("[AI] Luna AI Core initialized\n");

    /* ── 17. Scheduler (must be last — enables preemption) ───────────── */
    scheduler_init();
    uart_puts("[SCHED] Preemptive scheduler armed\n");

    /* ── 18. Enable interrupts ───────────────────────────────────────── */
    __asm__ volatile("sti");
    uart_puts("[LUNA] Interrupts enabled — system live\n");

    /* ── 19. Launch shell (Ring 0 CLI for now, Ring 3 later) ─────────── */
    shell_init();
    shell_run();

    /* Should never reach here */
    kernel_panic("kmain: shell_run() returned");
}
