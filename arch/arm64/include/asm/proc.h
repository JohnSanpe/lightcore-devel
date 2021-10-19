/* SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef _ASM_ARM_PROC_H_
#define _ASM_ARM_PROC_H_

#include <types.h>
#include <asm/barrier.h>

struct task;

static inline void arm32_domain_set(uint32_t domain)
{
	__asm__ __volatile__(
		"mcr p15, 0, %0, c3, c0, 0"
		:
		: "r" (domain)
		: "memory");
}

#define cpu_relax() barrier()

state arch_switch_task(struct task *prev, struct task *next);

#endif  /* _ASM_ARM_PROC_H_ */