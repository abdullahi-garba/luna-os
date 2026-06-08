; arch/x86/boot.asm — Luna OS Project AETERNA
; Multiboot2 compliant boot entry.
; Sets up:
;   1. Multiboot2 header with framebuffer tag (1024x768x32)
;   2. Bootstrap page directory for higher-half kernel mapping
;   3. 32KB kernel stack
;   4. Protected mode entry
;   5. Jump to kmain()
;
; Physical load: 0x00100000
; Virtual kernel: 0xC0000000 (higher-half)

KERNEL_VIRT_BASE equ 0xC0000000
KERNEL_PAGE_NUM  equ (KERNEL_VIRT_BASE >> 22)   ; PD index 768

section .multiboot2
align 8
mb2_header:
    dd 0xE85250D6                                   ; magic
    dd 0                                            ; architecture: i386 PM
    dd mb2_header_end - mb2_header                  ; header length
    dd -(0xE85250D6 + 0 + (mb2_header_end - mb2_header)) ; checksum

    ; ── Framebuffer tag ──────────────────────────────────
    align 8
    dw 5                ; tag type: framebuffer request
    dw 1                ; flags: optional (0 = required, 1 = optional)
    dd 20               ; tag size
    dd 1024             ; preferred width
    dd 768              ; preferred height
    dd 32               ; preferred depth (bpp)

    ; ── End tag ──────────────────────────────────────────
    align 8
    dw 0
    dw 0
    dd 8
mb2_header_end:

; ── Bootstrap Page Tables (identity + higher-half) ──────────────────────────
; We need TWO mappings before paging is enabled:
;   1. Identity map (0x00000000 → 0x00000000) so EIP keeps working after CR0 set
;   2. Higher-half map (0xC0000000 → 0x00000000)
; Both use 4MB pages (PSE) for simplicity at boot

section .bss
align 4096
boot_page_directory:
    resd 1024

section .data
align 4096
; PSE 4MB page flags: Present | Writable | PageSize(4MB)
BOOT_PDE_FLAGS equ 0x83

section .text
global boot_entry
extern kmain
extern __bss_start
extern __bss_end

boot_entry:
    ; ── Check multiboot2 magic ───────────────────────────
    cmp eax, 0x36D76289         ; multiboot2 bootloader magic in EAX
    jne .no_multiboot
    ; EBX = pointer to multiboot2 information structure (physical)

    ; ── Build bootstrap page directory ───────────────────
    ; Using 4MB PSE pages for fast boot mapping
    ; Identity map first 8MB (index 0)
    mov edi, boot_page_directory
    mov dword [edi + 0*4], 0x00000000 | BOOT_PDE_FLAGS
    mov dword [edi + 1*4], 0x00400000 | BOOT_PDE_FLAGS  ; 4MB identity

    ; Higher-half map: PD index 768 (0xC0000000) → physical 0x00000000
    mov dword [edi + KERNEL_PAGE_NUM*4],     0x00000000 | BOOT_PDE_FLAGS
    mov dword [edi + (KERNEL_PAGE_NUM+1)*4], 0x00400000 | BOOT_PDE_FLAGS

    ; ── Enable PSE (4MB pages) ───────────────────────────
    mov ecx, cr4
    or  ecx, 0x00000010     ; CR4.PSE = 1
    mov cr4, ecx

    ; ── Load page directory ──────────────────────────────
    mov ecx, boot_page_directory
    mov cr3, ecx

    ; ── Enable paging (CR0.PG + CR0.WP) ─────────────────
    mov ecx, cr0
    or  ecx, 0x80010000     ; PG=1, WP=1
    mov cr0, ecx

    ; ── Far jump to higher-half virtual address ──────────
    lea ecx, [.higher_half]
    jmp ecx

.higher_half:
    ; We are now executing at virtual 0xC0100000+
    ; Remove identity mapping (PD index 0) — security
    mov dword [boot_page_directory + 0*4], 0
    mov dword [boot_page_directory + 1*4], 0
    invlpg [0]

    ; ── Set up kernel stack ──────────────────────────────
    mov esp, stack_top

    ; ── Zero BSS section ────────────────────────────────
    mov edi, __bss_start
    mov ecx, __bss_end
    sub ecx, edi
    xor eax, eax
    rep stosb

    ; ── Call C++ global constructors ────────────────────
    call run_global_ctors

    ; ── Call kernel main ────────────────────────────────
    ; Pass multiboot2 info pointer (physical → virtual adjusted in kmain)
    push ebx            ; multiboot2 info structure pointer
    push eax            ; multiboot2 magic (for validation in kmain)
    call kmain

    ; kmain should never return
    cli
.hang:
    hlt
    jmp .hang

.no_multiboot:
    ; No multiboot2 magic — cannot boot
    cli
.hang2:
    hlt
    jmp .hang2

; ── C++ global constructor runner ────────────────────────────────────────────
extern __ctors_start
extern __ctors_end

run_global_ctors:
    mov edi, __ctors_start
.loop:
    cmp edi, __ctors_end
    jge .done
    call [edi]
    add edi, 4
    jmp .loop
.done:
    ret

; ── Kernel stack (32KB) ──────────────────────────────────────────────────────
section .bss
align 16
stack_bottom:
    resb 32768          ; 32KB kernel stack
stack_top:
