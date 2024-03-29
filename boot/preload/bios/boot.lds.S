/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright(c) 2021 Sanpe <sanpeqf@gmail.com>
 */

#include <size.h>
#define stack_size (SZ_1KiB * 8)

ENTRY(_start)

/* Declare storage space */
MEMORY {
    MBR (rx) : ORIGIN = 0x7c00,  LENGTH = 0x01be
    DPT (rw) : ORIGIN = 0x7dbe,  LENGTH = 0x0040
    SIG (rw) : ORIGIN = 0x7dfe,  LENGTH = 0x0002

    /* mem:  0x10000 - 0x7e00 = 0x8200 */
    /* disk:         512 * 62 = 0x7C00 */
    RAM (rwx) : ORIGIN = 0x7e00,  LENGTH = 0x7C00
}

SECTIONS {
    .setup : {
        PROVIDE(_ld_setup_start = .);
        KEEP(*(.setup))
        PROVIDE(_ld_esetup = .);
    } > MBR

    .signature : {
        SHORT(0xaa55)
    } > SIG

    .text : {
        . = ALIGN(4);
        PROVIDE(_ld_text_start = .);
        *(.text)
        *(.text*)
        . = ALIGN(4);
        PROVIDE(_ld_text_end = .);
    } > RAM

    .rodata : {
        PROVIDE(_ld_rodata_start = .);
        *(.rodata)
        *(.rodata*)
        PROVIDE(_ld_rodata_end = .);
    } > RAM

    .data : {
        PROVIDE(_ld_data_start = .);
        *(.data)
        *(.data*)
        PROVIDE(_ld_data_end = .);
    } > RAM

    .align : {
        PROVIDE(_ld_align_start = .);
        SHORT(0xaa55)
        . = ALIGN(512);
        PROVIDE(_ld_align_end = .);
    } > RAM

    .bss (NOLOAD) : {
        PROVIDE(_ld_bss_start = .);
        *(.bss)
        *(.bss*)
        PROVIDE(_ld_bss_end = .);
    } > RAM

    .stack (NOLOAD) : {
        . = ALIGN(4);
        PROVIDE(_ld_stack_start = .);
        . = _ld_stack_start + stack_size;
        . = ALIGN(4);
        PROVIDE(_ld_stack_end = .);
    } > RAM

    PROVIDE(_ld_block_start = (_ld_text_start - _ld_setup_start)/512);
    PROVIDE(_ld_block_num = (_ld_align_end - _ld_text_start)/512);
}
