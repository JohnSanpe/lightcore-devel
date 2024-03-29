/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright(c) 2021 Sanpe <sanpeqf@gmail.com>
 */

#include <printk.h>
#include <string.h>
#include <kernel.h>
#include <console.h>
#include <kclock.h>
#include <export.h>

const unsigned int printk_level_color[] = {
    [KLEVEL_EMERG]      = KERN_COLR_RED,
    [KLEVEL_ALERT]      = KERN_COLR_DARK_MAGENTA,
    [KLEVEL_CRIT]       = KERN_COLR_MAGENTA,
    [KLEVEL_ERR]        = KERN_COLR_YELLOW,
    [KLEVEL_WARNING]    = KERN_COLR_BLUE,
    [KLEVEL_NOTICE]     = KERN_COLR_CYAN,
    [KLEVEL_INFO]       = KERN_COLR_GREEN,
    [KLEVEL_DEBUG]      = KERN_COLR_DARK_GRAY,
    [KLEVEL_DEFAULT]    = KERN_COLR_DEFAULT,
};
EXPORT_SYMBOL(printk_level_color);

static inline char printk_get_level(const char *str)
{
    if (str[0] == KERN_SOH_ASCII && str[1])
        return str[1];
    return 0;
}

unsigned int printk_level(const char *str, const char **endptr)
{
    char value, klevel;

    for (klevel = KLEVEL_DEFAULT; *str; str += 2) {
        value = printk_get_level(str);
        if (!value)
            break;

        switch (value) {
            case '0' ... '9':
                klevel = value - '0';
                break;

            default:
                break;
        }
    }

    if (*endptr)
        *endptr = str;

    return klevel;
}
EXPORT_SYMBOL(printk_level);

static inline int printk_time(char *buffer, struct timespec *time)
{
    return sprintf(buffer, "[%5llu.%06lu] ",
        time->tv_sec, time->tv_nsec / 1000);
}

int vprintk(const char *fmt, va_list args)
{
    char buffer[256], *p = buffer;
    unsigned int klevel;
    int len;

    klevel = printk_level(fmt, &fmt);

#ifdef CONFIG_PRINTK_TIME
    struct timespec btime;
    btime = ns_to_timespec(kclock_get());
    if (strchr(fmt, '\n'))
        p += printk_time(p, &btime);
#endif

#ifdef CONFIG_PRINTK_COLOR
    p += sprintf(p, "\e[%dm", printk_level_color[klevel]);
#endif
    p += vsprintf(p, fmt, args);
#ifdef CONFIG_PRINTK_COLOR
    p += sprintf(p, "\e[0m");
#endif

    len = p - buffer;
    console_write(buffer, len);

    return len;
}
EXPORT_SYMBOL(vprintk);

int printk(const char *fmt, ...)
{
    va_list para;
    int length;

    va_start(para,fmt);
    length = vprintk(fmt, para);
    va_end(para);

    return length;
}
EXPORT_SYMBOL(printk);
