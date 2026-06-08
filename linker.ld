/* linker.ld — Luna OS x86 Higher-Half Kernel Linker Script
 * Kernel virtual base: 0xC0000000 (3GB mark)
 * Physical load base:  0x00100000 (1MB — above BIOS/VGA)
 * GRUB loads us at physical 0x100000; paging maps it to 0xC0100000
 */

ENTRY(boot_entry)

KERNEL_VIRT_BASE = 0xC0000000;
KERNEL_PHYS_BASE = 0x00100000;

SECTIONS
{
    /* ── Boot section loads at physical 1MB, but linked at VIRT ─── */
    . = KERNEL_VIRT_BASE + KERNEL_PHYS_BASE;

    /* Multiboot2 header must be in the first 32KB of the image */
    .multiboot2 ALIGN(8) :
    AT(ADDR(.multiboot2) - KERNEL_VIRT_BASE)
    {
        KEEP(*(.multiboot2))
    }

    /* Text (code) */
    .text ALIGN(4096) :
    AT(ADDR(.text) - KERNEL_VIRT_BASE)
    {
        *(.text)
        *(.text.*)
    }

    /* Read-only data */
    .rodata ALIGN(4096) :
    AT(ADDR(.rodata) - KERNEL_VIRT_BASE)
    {
        *(.rodata)
        *(.rodata.*)
    }

    /* C++ global constructors */
    .ctors ALIGN(4) :
    AT(ADDR(.ctors) - KERNEL_VIRT_BASE)
    {
        __ctors_start = .;
        KEEP(*(.init_array))
        KEEP(*(SORT_BY_INIT_PRIORITY(.init_array.*)))
        __ctors_end = .;
    }

    /* Initialized data */
    .data ALIGN(4096) :
    AT(ADDR(.data) - KERNEL_VIRT_BASE)
    {
        *(.data)
        *(.data.*)
    }

    /* BSS — zero-initialized */
    .bss ALIGN(4096) :
    AT(ADDR(.bss) - KERNEL_VIRT_BASE)
    {
        __bss_start = .;
        *(.bss)
        *(.bss.*)
        *(COMMON)
        __bss_end = .;
    }

    /* Kernel end marker */
    __kernel_end = .;
    __kernel_end_phys = __kernel_end - KERNEL_VIRT_BASE;

    /* Discard debug and exception handling junk */
    /DISCARD/ :
    {
        *(.comment)
        *(.eh_frame)
        *(.note.*)
    }
}
