/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright(c) 2021 Sanpe <sanpeqf@gmail.com>
 */

#include <kboot.h>
#include <driver/video/vesa.h>

#include <asm/io.h>

#define xres    80
#define yres    25

struct _vram_text{
    struct{
    uint8_t ch;
    uint8_t att;
    } block[yres][xres];
} __packed;

#define vram_text_base  0xb8000
#define vram_text       ((struct _vram_text *)vram_text_base)

static unsigned char pos_x, pos_y;

static inline void video_clear(int pos_y, int len)
{
    int pos_x;
    for(; len--; pos_y++)
    for(pos_x = 0; pos_x < 80; pos_x++) {
        vram_text->block[pos_y][pos_x].ch = '\0';
        vram_text->block[pos_y][pos_x].att = 0x07;
    }
}

static inline void video_cursor(const char pos_x, const char pos_y)
{
    uint16_t cursor = pos_x + (pos_y * 80);

    outb(VESA_CRT_IC, VESA_CRTC_CURSOR_HI);
    outb(VESA_CRT_DC, cursor >> 8);
    outb(VESA_CRT_IC, VESA_CRTC_CURSOR_LO);
    outb(VESA_CRT_DC, cursor);
}

static inline void video_roll()
{
    memmove(&vram_text->block[0][0], &vram_text->block[1][0], 2 * xres * (yres - 1));
    video_clear(yres - 1, 1);
}

void console_print(const char *str)
{   
    char ch;
    
    while((ch = *str++) != '\0') {
        if (pos_y >= yres)
            video_roll(pos_y--);

        if (ch == '\n') {
            pos_y++;
            pos_x = 0;
            continue;
        }
        
        vram_text->block[pos_y][pos_x++].ch = ch;
        if (pos_x >= xres) {
            pos_y++;
            pos_x = 0;
        }
        
        video_cursor(pos_x, pos_y);
    }
}

void console_clear()
{
    video_clear(0, yres);
    video_cursor(0, 0);
}
