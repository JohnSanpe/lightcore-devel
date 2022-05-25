/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright(c) 2021 Sanpe <sanpeqf@gmail.com>
 */

#include <ctype.h>
#include <string.h>
#include <initcall.h>
#include <kshell.h>

static void usage(struct kshell_context *ctx)
{
    kshell_printf(ctx, "usage: if/elif [option] condition {commands} else {commands}\n");
    kshell_printf(ctx, "\t-h  display this message\n");
}

static state cond_main(struct kshell_context *ctx, int argc, char *argv[])
{
    unsigned int count;
    unsigned long condition;
    state retval = -ENOERR;

    for (count = 1; count < argc; ++count) {
        char *para = argv[count];

        if (para[0] != '-')
            break;

        switch (para[1]) {
            case 'h':
                goto usage;

            default:
                goto finish;
        }
    }

finish:
    if (argc < count + 2)
        goto usage;

    condition = axtol(argv[count++]);
    if (!condition)
        retval = kshell_system(ctx, argv[count++]);

    else for (count++; count < argc - 1; count += 2) {
        if (!strcmp(argv[count], "elif")) {
            condition = axtol(argv[++count]);
            if (condition)
                continue;
            retval = kshell_system(ctx, argv[++count]);
            break;
        } else if (!strcmp(argv[count], "else")) {
            retval = kshell_system(ctx, argv[++count]);
            break;
        } else
            goto usage;
    }

    return retval;

usage:
    usage(ctx);
    return -EINVAL;
}

static struct kshell_command cond_cmd = {
    .name = "if",
    .desc = "condition execute commands",
    .exec = cond_main,
};

static state cond_init(void)
{
    return kshell_register(&cond_cmd);
}
kshell_initcall(cond_init);
