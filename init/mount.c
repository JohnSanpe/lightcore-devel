/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright(c) 2021 Sanpe <sanpeqf@gmail.com>
 */

#define MODULE_NAME "VFL"
#define pr_fmt(fmt) MODULE_NAME ": " fmt

#include <init.h>
#include <filesystem.h>
#include <printk.h>

static state __init auto_mount(const char *devn, enum mount_flags flags)
{
    struct filesystem_type *fs;
    state ret = -EINVAL;

    spin_lock(&filesystem_lock);
    list_for_each_entry(fs, &filesystem_list, list) {
        const char *name = fs->name;

        pr_info("Try mount '%s' on '%s'\n", name, devn);

        ret = kernel_mount(devn, name, flags);
        switch (ret) {
            case -ENOERR:
                goto exit;
            case -EINVAL:
                continue;
            default:
                break;
        }

        pr_crit("Can't mount root device ");

    }

exit:
    spin_unlock(&filesystem_lock);
    return ret;
}

#ifdef CONFIG_BLOCK
static state __init mount_block_root(void)
{
    return -ENOERR;
}
#endif

#ifdef CONFIG_ROMDISK
static state __init mount_romdisk_root(void)
{
    return -ENOERR;
}
#endif

state __init vfl_init_root(void)
{
    state ret;

#ifdef CONFIG_BLOCK
    ret = mount_block_root();
    if (!ret)
        return -ENOERR;
    pr_warn("Unable to mount rootfs on block (%d)\n", ret);
#endif

#ifdef CONFIG_ROMDISK
    ret = mount_romdisk_root();
    if (!ret)
        return -ENOERR;
    pr_warn("Unable to mount rootfs on romdisk (%d)\n", ret);
#endif

    return ret;
}
