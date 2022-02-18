/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright(c) 2021 Sanpe <sanpeqf@gmail.com>
 */

#define pr_fmt(fmt) "vmalloc: " fmt

#include <vmalloc.h>
#include <numa.h>
#include <mm/vmem.h>
#include <mm/vmap.h>
#include <mm/page.h>
#include <kmalloc.h>
#include <printk.h>
#include <panic.h>
#include <export.h>

struct vmalloc_area {
    struct vmem_area vmem;
    struct page **page_map;
    unsigned long page_nr;
};

#define vmem_to_vmalloc(vmp) \
    container_of(vmp, struct vmalloc_area, vmem)

static struct kcache *vmalloc_cache;
static atomic_t vmalloc_page_nr;

static void release_pages(struct page **page_area, unsigned long page_nr)
{
    unsigned long count;
    struct page *page;

    for (count = 0; count < page_nr; ++count) {
        page = page_area[count];
        BUG_ON(!page);
        page_free(page);
    }

    atomic_sub(&vmalloc_page_nr, page_nr);
}

static void vfree_pages_internal(void *block, bool free_pages)
{
    struct vmalloc_area *vmalloc;
    struct vmem_area *vmem;

    if (WARN_ON(!block))
        return;

    if ((size_t)block < CONFIG_HIGHMAP_OFFSET)
        return;

    vmem = vmem_area_find((size_t)block);
    if (!vmem) {
        pr_err("unmap not found\n");
        return;
    }

    vmalloc = vmem_to_vmalloc(vmem);
    if (free_pages) {
        release_pages(vmalloc->page_map, vmalloc->page_nr);
        kfree(vmalloc->page_map);
    }

    vmem_area_free(vmem);
    vunmap_range(&init_mm, vmalloc->vmem.addr, vmalloc->vmem.size);
    kcache_free(vmalloc_cache, vmalloc);
}

void vfree_pages(void *block)
{
    vfree_pages_internal(block, false);
}
EXPORT_SYMBOL(vfree_pages);

/**
 * vfree - release virtually contiguous memory
 * @addr: memory address
 */
void vfree(void *block)
{
    vfree_pages_internal(block, true);
}
EXPORT_SYMBOL(vfree);

/**
 * vmalloc_pages - mapping page arrays to contiguous virtual addresses.
 * @pages: array of page pointers
 * @count: number of pages to map
 * @flags: vmem mapping parameters
 */
void *vmalloc_pages(struct page **pages, unsigned int count, gvm_t flags)
{
    struct vmalloc_area *vmalloc;
    size_t size = (unsigned long)count << PAGE_SHIFT;

    vmalloc = kcache_zalloc(vmalloc_cache, GFP_KERNEL);
    if (unlikely(!vmalloc))
        return NULL;

    if (vmem_area_alloc(&vmalloc->vmem, size))
        goto err_free;

    if (vmap_pages(&init_mm, pages, vmalloc->vmem.addr, size, 0, PGDIR_SHIFT))
        goto err_vmem;

    vmalloc->page_nr = count;
    vmalloc->page_map = pages;
    return (void *)vmalloc->vmem.addr;

err_vmem:
    vmem_area_free(&vmalloc->vmem);
err_free:
    kcache_free(vmalloc_cache, vmalloc);
    return NULL;
}
EXPORT_SYMBOL(vmalloc_pages);

static state alloc_pages(struct vmalloc_area *area, gfp_t gfp)
{
    unsigned int count;
    struct page *page;

    atomic_add(&vmalloc_page_nr, area->page_nr);
    for (count = 0; count < area->page_nr; ++count) {
        page = page_alloc(0, gfp);
        if (unlikely(!page))
            goto fail;
        area->page_map[count] = page;
    }

    return -ENOERR;

fail:
    release_pages(area->page_map, count);
    pr_warn("Page allocation failure\n");
    return -ENOMEM;
}

void *vmalloc_numa(size_t size, size_t align, gfp_t gfp, int numa)
{
    struct vmalloc_area *vmalloc;
    struct page **page_area;
    unsigned long page_nr;

    align = max(align, PAGE_SIZE);
    size = align_high(size, align);

    vmalloc = kcache_zalloc(vmalloc_cache, GFP_KERNEL);
    if (unlikely(!vmalloc))
        return NULL;

    page_nr = size >> PAGE_SHIFT;
    page_area = kmalloc(sizeof(*page_area) * page_nr, GFP_KERNEL);
    if (unlikely(!page_area))
        goto err_vmalloc;

    if (vmem_area_alloc(&vmalloc->vmem, size))
        goto err_area;

    vmalloc->page_nr = page_nr;
    vmalloc->page_map = page_area;

    if (alloc_pages(vmalloc, gfp))
        goto err_vmem;

    if (vmap_pages(&init_mm, page_area, vmalloc->vmem.addr, size, 0, PAGE_SHIFT))
        goto err_pages;

    return (void *)vmalloc->vmem.addr;

err_pages:
    release_pages(page_area, page_nr);
err_vmem:
    vmem_area_free(&vmalloc->vmem);
err_area:
    kfree(page_area);
err_vmalloc:
    kcache_free(vmalloc_cache, vmalloc);
    return NULL;
}
EXPORT_SYMBOL(vmalloc_numa);

/**
 * vmalloc - allocate virtually contiguous memory
 * @size: allocation size
 */
void *vmalloc(size_t size)
{
    return vmalloc_numa(size, MSIZE, GFP_KERNEL, NUMA_NONE);
}
EXPORT_SYMBOL(vmalloc);

/**
 * vzalloc - allocate virtually contiguous memory and set to zero
 * @size: allocation size
 */
void *vzalloc(size_t size)
{
    return vmalloc_numa(size, MSIZE, GFP_KERNEL | GFP_ZERO, NUMA_NONE);
}
EXPORT_SYMBOL(vzalloc);

void __init vmalloc_init(void)
{
    vmalloc_cache = kcache_create("vmalloc",
        sizeof(struct vmalloc_area), KCACHE_PANIC);
}
