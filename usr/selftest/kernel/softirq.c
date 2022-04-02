/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright(c) 2021 Sanpe <sanpeqf@gmail.com>
 */

#include <initcall.h>
#include <softirq.h>
#include <selftest.h>

#define TEST_LOOP 100

static void softirq_test_handle(void *pdata)
{
    spin_unlock(pdata);
}

static state softirq_test_testing(void *pdata)
{
    unsigned int count;
    state ret;

    SPIN_LOCK(lock);
    DEFINE_SOFTIRQ(softirq_test, softirq_test_handle, &lock, 0);

    ret = softirq_regsiter(&softirq_test);
    if (ret)
        return ret;

    spin_lock(&lock);
    for (count = 0; count < TEST_LOOP; ++count) {
        kshell_printf("softirq test%02u: ", count);
        softirq_pending(&softirq_test);
        spin_lock(&lock);
        kshell_printf("lock ack\n");
    }

    softirq_unregister(&softirq_test);
    return -ENOERR;
}

static struct selftest_command softirq_command = {
    .group = "kernel",
    .name = "softirq",
    .desc = "softirq unit test",
    .testing = softirq_test_testing,
};

static state selftest_softirq_init(void)
{
    return selftest_register(&softirq_command);
}
kshell_initcall(selftest_softirq_init);
