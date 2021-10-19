/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright(c) 2021 Sanpe <sanpeqf@gmail.com>
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>

static void error(const char *str)
{
    fprintf(stderr, str);
    fprintf(stderr, "\n");
    exit(1);
}

int main(int argc, char *argv[])
{
    if (argc != 3) {
        printf("usage: mkpiggy segment file\n");
        return -1;
    }
    
    printf("/* SPDX-License-Identifier: GPL-2.0-or-later */\n\n");
    printf("    .section %s, \"a\"\n", argv[1]);
	printf(".incbin \"%s\"\n", argv[2]);

    return 0;
}