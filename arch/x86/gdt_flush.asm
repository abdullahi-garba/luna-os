; arch/x86/gdt_flush.asm — GDT and TSS flush routines
; Called from gdt.c after building the GDT table.
; Executes lgdt, performs a far jump to reload CS with the kernel code selector,
; then reloads all data segment registers.

global gdt_flush
global tss_flush

; void gdt_flush(uint32_t gdt_ptr_addr)
gdt_flush:
    mov  eax, [esp+4]    ; GDT_Ptr address passed as argument
    lgdt [eax]           ; load the GDT register

    ; Far jump: reload CS with Ring 0 code selector (GDT index 1 = 0x08)
    ; This flushes the pipeline and updates the code segment descriptor cache
    jmp  0x08:.reload_cs

.reload_cs:
    ; Reload all data segment registers with Ring 0 data selector (0x10)
    mov  ax, 0x10
    mov  ds, ax
    mov  es, ax
    mov  fs, ax
    mov  gs, ax
    mov  ss, ax
    ret

; void tss_flush(void)
; Loads the TSS selector into the task register (TR)
tss_flush:
    ; TSS is at GDT index 5 = selector 0x28, RPL=0
    ; ltr loads the task register
    mov  ax, 0x28
    ltr  ax
    ret
