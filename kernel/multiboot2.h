/* kernel/multiboot2.h — Luna OS Multiboot2 Tag Definitions
 * Covers all standard multiboot2 information tags we parse in kmain().
 */

#ifndef LUNA_MULTIBOOT2_H
#define LUNA_MULTIBOOT2_H

#include "../include/types.h"

#define MULTIBOOT2_MAGIC        0x36D76289U
#define MULTIBOOT2_BOOTLOADER   0xE85250D6U

/* ── Tag type IDs ────────────────────────────────────────────────────────── */
#define MB2_TAG_END             0
#define MB2_TAG_CMDLINE         1
#define MB2_TAG_BOOTLOADER      2
#define MB2_TAG_MODULE          3
#define MB2_TAG_BASIC_MEMINFO   4
#define MB2_TAG_BOOTDEV         5
#define MB2_TAG_MMAP            6
#define MB2_TAG_VBE             7
#define MB2_TAG_FRAMEBUFFER     8
#define MB2_TAG_ELF_SECTIONS    9
#define MB2_TAG_APM             10
#define MB2_TAG_ACPI_OLD        14
#define MB2_TAG_ACPI_NEW        15

/* ── Base tag header (every tag starts with these 8 bytes) ──────────────── */
typedef struct PACKED {
    uint32_t type;
    uint32_t size;
} MB2_Tag;

/* ── Multiboot2 info structure header ───────────────────────────────────── */
typedef struct PACKED {
    uint32_t total_size;
    uint32_t reserved;
    /* tags follow immediately after */
} MB2_Info;

/* ── Memory map ─────────────────────────────────────────────────────────── */
#define MB2_MMAP_AVAILABLE      1
#define MB2_MMAP_RESERVED       2
#define MB2_MMAP_ACPI_RECLAIM   3
#define MB2_MMAP_NVS            4
#define MB2_MMAP_BADRAM         5

typedef struct PACKED {
    uint64_t base_addr;
    uint64_t length;
    uint32_t type;
    uint32_t reserved;
} MB2_MmapEntry;

typedef struct PACKED {
    uint32_t type;          /* MB2_TAG_MMAP = 6 */
    uint32_t size;
    uint32_t entry_size;
    uint32_t entry_version;
    /* MB2_MmapEntry entries[] follow */
} MB2_TagMmap;

/* ── Framebuffer ─────────────────────────────────────────────────────────── */
#define MB2_FB_TYPE_INDEXED     0
#define MB2_FB_TYPE_RGB         1
#define MB2_FB_TYPE_EGA_TEXT    2

typedef struct PACKED {
    uint32_t type;          /* MB2_TAG_FRAMEBUFFER = 8 */
    uint32_t size;
    uint64_t framebuffer_addr;   /* physical address of framebuffer */
    uint32_t framebuffer_pitch;  /* bytes per row */
    uint32_t framebuffer_width;
    uint32_t framebuffer_height;
    uint8_t  framebuffer_bpp;    /* bits per pixel */
    uint8_t  framebuffer_type;   /* MB2_FB_TYPE_RGB typically */
    uint16_t reserved;
} MB2_TagFramebuffer;

/* ── Command line ────────────────────────────────────────────────────────── */
typedef struct PACKED {
    uint32_t type;          /* MB2_TAG_CMDLINE = 1 */
    uint32_t size;
    char     string[];      /* null-terminated command line */
} MB2_TagCmdline;

/* ── Basic memory info ───────────────────────────────────────────────────── */
typedef struct PACKED {
    uint32_t type;          /* MB2_TAG_BASIC_MEMINFO = 4 */
    uint32_t size;
    uint32_t mem_lower;     /* KB below 1MB */
    uint32_t mem_upper;     /* KB above 1MB */
} MB2_TagBasicMeminfo;

/* ── Tag iterator ────────────────────────────────────────────────────────── */
/* Usage:
 *   MB2_Tag* tag = MB2_FIRST_TAG(mb2_info);
 *   while (tag->type != MB2_TAG_END) {
 *       if (tag->type == MB2_TAG_FRAMEBUFFER) { ... }
 *       tag = MB2_NEXT_TAG(tag);
 *   }
 */
#define MB2_FIRST_TAG(info) \
    ((MB2_Tag*)((uint8_t*)(info) + sizeof(MB2_Info)))

#define MB2_NEXT_TAG(tag) \
    ((MB2_Tag*)(((uint8_t*)(tag) + (tag)->size + 7) & ~7))

#endif /* LUNA_MULTIBOOT2_H */
