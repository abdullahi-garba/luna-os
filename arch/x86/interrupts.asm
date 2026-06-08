; arch/x86/interrupts.asm — Luna OS ISR/IRQ stubs + syscall entry
; Generates all 32 exception stubs, 16 IRQ stubs, and the int 0x80 syscall stub.
; Each stub saves CPU state, calls C handler, restores state, and irets.

extern isr_handler    ; arch/x86/idt.c
extern irq_handler    ; arch/x86/idt.c
extern syscall_dispatch ; kernel/syscall.c

global idt_flush

; ── lidt wrapper ─────────────────────────────────────────────────────────────
idt_flush:
    mov eax, [esp+4]
    lidt [eax]
    ret

; ── Common ISR stub (exceptions) ─────────────────────────────────────────────
; Stack on entry: [eip, cs, eflags] pushed by CPU
;                 [err_code, int_no] pushed by our stub
; We then pusha and call the C handler.

%macro ISR_NOERR 1
global isr%1
isr%1:
    cli
    push dword 0        ; dummy error code
    push dword %1       ; interrupt number
    jmp isr_common
%endmacro

%macro ISR_ERR 1
global isr%1
isr%1:
    cli
                        ; CPU already pushed error code
    push dword %1       ; interrupt number
    jmp isr_common
%endmacro

; CPU exceptions: which ones push error codes?
ISR_NOERR  0   ; #DE Division by Zero
ISR_NOERR  1   ; #DB Debug
ISR_NOERR  2   ; NMI
ISR_NOERR  3   ; #BP Breakpoint
ISR_NOERR  4   ; #OF Overflow
ISR_NOERR  5   ; #BR Bound Range Exceeded
ISR_NOERR  6   ; #UD Invalid Opcode
ISR_NOERR  7   ; #NM Device Not Available
ISR_ERR    8   ; #DF Double Fault       (error code = 0)
ISR_NOERR  9   ; Coprocessor Segment Overrun
ISR_ERR   10   ; #TS Invalid TSS
ISR_ERR   11   ; #NP Segment Not Present
ISR_ERR   12   ; #SS Stack Fault
ISR_ERR   13   ; #GP General Protection Fault
ISR_ERR   14   ; #PF Page Fault
ISR_NOERR 15   ; Reserved
ISR_NOERR 16   ; #MF x87 FPU Error
ISR_ERR   17   ; #AC Alignment Check
ISR_NOERR 18   ; #MC Machine Check
ISR_NOERR 19   ; #XF SIMD Float
ISR_NOERR 20   ; Virtualization
ISR_NOERR 21
ISR_NOERR 22
ISR_NOERR 23
ISR_NOERR 24
ISR_NOERR 25
ISR_NOERR 26
ISR_NOERR 27
ISR_NOERR 28
ISR_NOERR 29
ISR_ERR   30   ; #SX Security Exception
ISR_NOERR 31

isr_common:
    pusha               ; push edi,esi,ebp,esp,ebx,edx,ecx,eax
    push ds
    push es
    push fs
    push gs

    ; Load kernel data segments
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    push esp            ; push pointer to CPU_Regs struct
    call isr_handler
    add esp, 4          ; pop the pointer arg

    pop gs
    pop fs
    pop es
    pop ds
    popa
    add esp, 8          ; pop int_no and err_code
    iret

; ── Hardware IRQ stubs ────────────────────────────────────────────────────────
%macro IRQ_STUB 2
global irq%1
irq%1:
    cli
    push dword 0        ; no error code
    push dword %2       ; interrupt number (32 + IRQ number)
    jmp irq_common
%endmacro

IRQ_STUB  0, 32    ; PIT timer
IRQ_STUB  1, 33    ; PS/2 keyboard
IRQ_STUB  2, 34    ; cascade
IRQ_STUB  3, 35    ; COM2
IRQ_STUB  4, 36    ; COM1
IRQ_STUB  5, 37    ; LPT2
IRQ_STUB  6, 38    ; floppy
IRQ_STUB  7, 39    ; LPT1 / spurious
IRQ_STUB  8, 40    ; CMOS RTC
IRQ_STUB  9, 41    ; free / ACPI
IRQ_STUB 10, 42    ; free
IRQ_STUB 11, 43    ; free
IRQ_STUB 12, 44    ; PS/2 mouse
IRQ_STUB 13, 45    ; FPU/coprocessor
IRQ_STUB 14, 46    ; primary ATA
IRQ_STUB 15, 47    ; secondary ATA

irq_common:
    pusha
    push ds
    push es
    push fs
    push gs

    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    push esp
    call irq_handler
    add esp, 4

    pop gs
    pop fs
    pop es
    pop ds
    popa
    add esp, 8
    iret

; ── Syscall gate: int 0x80 ────────────────────────────────────────────────────
; Calling convention (Linux-compatible):
;   EAX = syscall number
;   EBX = arg1, ECX = arg2, EDX = arg3
;   Returns: EAX = result

global isr128
isr128:
    cli
    push dword 0        ; no error code
    push dword 0x80     ; int number

    pusha
    push ds
    push es
    push fs
    push gs

    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    push esp            ; pointer to full CPU_Regs
    call syscall_dispatch
    add esp, 4
    ; EAX from C function becomes the return value

    pop gs
    pop fs
    pop es
    pop ds
    popa
    add esp, 8
    iret
