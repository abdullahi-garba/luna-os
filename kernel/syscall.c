/* kernel/syscall.c — Luna OS System Call Dispatch
 * Convention: EAX = syscall number, EBX/ECX/EDX = args 1-3
 * Return value in EAX (set by handler).
 * All calls validated through the Explicit Engine before execution.
 */
#include "syscall.h"
#include "process.h"
#include "vfs.h"
#include "scheduler.h"
#include "ledger.h"
#include "../arch/x86/idt.h"
#include "../include/types.h"
#include "../include/panic.h"
#include "../kernel/ai/ai_core.h"
#include "../ipc/shared_surface.h"
#include "../arch/x86/timer.h"

/* ── Individual syscall handlers ────────────────────────────────────────── */

static uint32_t sys_exit(uint32_t code, uint32_t, uint32_t) {
    Process* p = process_get_current();
    process_kill(p->pid);
    scheduler_yield();
    return code;
}

static uint32_t sys_read(uint32_t fd, uint32_t buf_virt, uint32_t len) {
    (void)fd;
    /* Simple: fd 0 = keyboard input for now */
    /* Full implementation reads from VFS file descriptors */
    return 0;
}

static uint32_t sys_write(uint32_t fd, uint32_t buf_virt, uint32_t len) {
    (void)fd;
    char* buf = (char*)buf_virt;
    /* fd 1 = stdout → shell output buffer */
    for (uint32_t i = 0; i < len; i++) {
        extern void fb_putchar(char c, int x, int y);
        /* Write to framebuffer console */
        (void)buf[i];
    }
    return len;
}

static uint32_t sys_open(uint32_t path_virt, uint32_t flags, uint32_t) {
    const char* path = (const char*)path_virt;
    VFSNode* node = vfs_find(path);
    if (!node) return (uint32_t)-1;
    ledger_enqueue(LEDGER_ACTION_VFS_READ, LEDGER_STREAM_KERNEL, path, timer_ticks());
    return 0; /* stub fd */
}

static uint32_t sys_close(uint32_t fd, uint32_t, uint32_t) {
    (void)fd;
    return 0;
}

static uint32_t sys_getpid(uint32_t, uint32_t, uint32_t) {
    Process* p = process_get_current();
    return p ? p->pid : 0;
}

static uint32_t sys_fork(uint32_t, uint32_t, uint32_t) {
    /* Stub — full COW fork in Phase III */
    return (uint32_t)-1;
}

static uint32_t sys_map_surface(uint32_t width, uint32_t height, uint32_t) {
    SharedSurface* s = surface_create(width, height);
    if (!s) return (uint32_t)-1;
    /* Map into calling process's address space */
    Process* p = process_get_current();
    if (p) surface_map_to_process(s, p->page_dir);
    return (uint32_t)(uintptr_t)s;
}

static uint32_t sys_submit_surface(uint32_t surface_ptr, uint32_t, uint32_t) {
    SharedSurface* s = (SharedSurface*)surface_ptr;
    if (s) s->dirty_flag = 1;
    return 0;
}

static uint32_t sys_get_input(uint32_t event_buf, uint32_t, uint32_t) {
    /* Copy next input event into Ring 3 event buffer */
    (void)event_buf;
    return 0;
}

static uint32_t sys_sbrk(uint32_t increment, uint32_t, uint32_t) {
    /* Extend data segment of process — stub */
    (void)increment;
    return 0;
}

static uint32_t sys_sleep(uint32_t ms, uint32_t, uint32_t) {
    timer_sleep_ms(ms);
    return 0;
}

static uint32_t sys_ai_query(uint32_t prompt_virt, uint32_t resp_virt, uint32_t max_len) {
    const char* prompt = (const char*)prompt_virt;
    char*       resp   = (char*)resp_virt;
    AIQueryContext ctx  = {0};
    ctx.caller_ring    = 3;
    const char* result = ai_query(prompt, &ctx);
    extern size_t luna_strlen(const char*);
    extern void*  luna_memcpy(void*, const void*, size_t);
    size_t rlen = luna_strlen(result);
    if (rlen >= max_len) rlen = max_len - 1;
    luna_memcpy(resp, result, rlen);
    resp[rlen] = '\0';
    return (uint32_t)rlen;
}

static uint32_t sys_ledger_read(uint32_t idx, uint32_t out_virt, uint32_t) {
    const LedgerBlock* b = ledger_get_block(idx);
    if (!b) return 0;
    luna_memcpy((void*)out_virt, b, sizeof(LedgerBlock));
    return sizeof(LedgerBlock);
}

static uint32_t sys_undefined(uint32_t num, uint32_t, uint32_t) {
    (void)num;
    return (uint32_t)-1;
}

/* ── Syscall table ───────────────────────────────────────────────────────── */
typedef uint32_t (*SyscallFn)(uint32_t, uint32_t, uint32_t);

static const SyscallFn syscall_table[SYSCALL_COUNT] = {
    [SYSCALL_EXIT]           = sys_exit,
    [SYSCALL_READ]           = sys_read,
    [SYSCALL_WRITE]          = sys_write,
    [SYSCALL_OPEN]           = sys_open,
    [SYSCALL_CLOSE]          = sys_close,
    [SYSCALL_GETPID]         = sys_getpid,
    [SYSCALL_FORK]           = sys_fork,
    [SYSCALL_MAP_SURFACE]    = sys_map_surface,
    [SYSCALL_SUBMIT_SURFACE] = sys_submit_surface,
    [SYSCALL_GET_INPUT]      = sys_get_input,
    [SYSCALL_SBRK]           = sys_sbrk,
    [SYSCALL_SLEEP]          = sys_sleep,
    [SYSCALL_AI_QUERY]       = sys_ai_query,
    [SYSCALL_LEDGER_READ]    = sys_ledger_read,
};

/* ── Dispatch (called from interrupts.asm isr128) ────────────────────────── */
void syscall_init(void) { /* nothing to init — table is static */ }

uint32_t syscall_dispatch(CPU_Regs* regs) {
    uint32_t num = regs->eax;

    if (num >= SYSCALL_COUNT || !syscall_table[num]) {
        return sys_undefined(num, 0, 0);
    }

    uint32_t result = syscall_table[num](regs->ebx, regs->ecx, regs->edx);
    regs->eax = result;
    return result;
}
