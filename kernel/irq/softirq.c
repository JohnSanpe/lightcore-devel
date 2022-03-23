/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright(c) 2021 Sanpe <sanpeqf@gmail.com>
 */

#define MODULE_NAME "softirq"
#define pr_fmt(fmt) MODULE_NAME ": " fmt

#include <linkage.h>
#include <kmalloc.h>
#include <softirq.h>
#include <export.h>
#include <panic.h>

#define SOFTIRQ_LIMIT   CONFIG_SOFTIRQ_LIMIT
#define SOFTIRQ_RETRY   16

static struct kcache *softirq_cache;
static struct idr_root *softirq_idr;

asmlinkage __visible __softirq_entry void softirq_handle(void)
{
    unsigned int max_restart = SOFTIRQ_RETRY;
    struct irq_cpustat *stat = thiscpu_ptr(&irq_stat);
    unsigned long *pending = stat->softirq_pending;
    unsigned int softirq_bit;

restart:
    while ((softirq_bit = find_first_bit(pending, SOFTIRQ_LIMIT)) < SOFTIRQ_LIMIT) {
        struct softirq *softirq;

        softirq = idr_find(softirq_idr, softirq_bit + 1);
        if (BUG_ON(!softirq))
            continue;

        if (!softirq_test_periodic(softirq))
            bit_clr(pending, softirq_bit);

        if (!softirq_test_irq_safe(softirq))
            softirq->entry(softirq->pdata);
        else {
            irq_local_enable();
            softirq->entry(softirq->pdata);
            irq_local_disable();
        }
    }

    if (!bitmap_empty(stat->softirq_pending, SOFTIRQ_LIMIT)) {
        if (max_restart--)
            goto restart;
    }
}

void noinline softirq_entry(void)
{

}

void noinline softirq_exit(void)
{
    softirq_handle();
}

static inline void raise_softirq(unsigned int irqnr)
{
    struct irq_cpustat *stat = thiscpu_ptr(&irq_stat);
    bit_set(stat->softirq_pending, irqnr - 1);
}

static inline void decline_softirq(unsigned int irqnr)
{
    struct irq_cpustat *stat = thiscpu_ptr(&irq_stat);
    bit_clr(stat->softirq_pending, irqnr - 1);
}

/**
 * softirq_pending - pending a softirq
 * @irq: the softirq to pending
 */
state softirq_pending(struct softirq *irq)
{
    irqflags_t irqflags;

    if (BUG_ON(!irq || !irq->entry))
        return -EINVAL;

    irqflags = irq_local_save();
    raise_softirq(irq->node.index);
    irq_local_restore(irqflags);

    return -ENOERR;
}
EXPORT_SYMBOL(softirq_pending);

/**
 * softirq_clear - clear a softirq
 * @irq: the softirq to clear
 */
void softirq_clear(struct softirq *irq)
{
    irqflags_t irqflags;

    if (BUG_ON(!irq))
        return;

    irqflags = irq_local_save();
    decline_softirq(irq->node.index);
    irq_local_restore(irqflags);
}
EXPORT_SYMBOL(softirq_clear);

/**
 * softirq_regsiter - regsiter a softirq
 * @irq: the softirq to regsiter
 */
state softirq_regsiter(struct softirq *irq)
{
    unsigned long id;

    if (BUG_ON(!irq || !irq->entry))
        return -EINVAL;

    id = idr_node_alloc_cyclic_max(softirq_idr, &irq->node, irq, SOFTIRQ_LIMIT);
    if (id == IDR_NONE)
        return -ENOMEM;

    return -ENOERR;
}
EXPORT_SYMBOL(softirq_regsiter);

/**
 * softirq_unregister - unregsiter a softirq
 * @irq: the softirq to unregsiter
 */
void softirq_unregister(struct softirq *irq)
{
    softirq_clear(irq);
    idr_node_free(softirq_idr, irq->node.index);
}
EXPORT_SYMBOL(softirq_unregister);

/**
 * softirq_create - Create a softirq node
 * @name: the name of softirq
 * @entry: the softirq handle entry
 * @pdata: handle entry pdata
 * @flags: softirq flags
 */
struct softirq *softirq_create(const char *name, softirq_entry_t entry, void *pdata, unsigned long flags)
{
    struct softirq *irq;

    irq = kcache_zalloc(softirq_cache, GFP_KERNEL);
    if (unlikely(!irq))
        return NULL;

    irq->name = name;
    irq->entry = entry;
    irq->pdata = pdata;
    irq->flags = flags;

    return irq;
}
EXPORT_SYMBOL(softirq_create);

/**
 * softirq_destroy - Destroy a softirq node
 * @irq: the softirq to destroy
 */
void softirq_destroy(struct softirq *irq)
{
    softirq_unregister(irq);
    kcache_free(softirq_cache, irq);
}
EXPORT_SYMBOL(softirq_destroy);

void __init softirq_init(void)
{
    softirq_cache = KCACHE_CREATE(struct softirq, KCACHE_PANIC);
    softirq_idr = idr_create(0);
}
