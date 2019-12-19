/* -*- mode: C; c-file-style: "Linux" -*-
 *
 * Copyright (c) 2018 Luke Huang <lukehuang.ca@gmail.com>
 */

/**
 * \file page_fault.c
 * \author Luke Huang
 */

#include <linux/kernel.h>
#include <linux/linkage.h>
#include <linux/sched.h>

#include <asm/ptrace.h>

#ifdef CONFIG_X86_64
#  include "arch/x86_64/kutils.h"
#else

#endif

#define PGFAULT_MODIFIER asmlinkage
typedef PGFAULT_MODIFIER void (*do_pgfault_func_t)(struct pt_regs *,
                                                   unsigned long);

#define PGFAULT_YA_COUNT	95597
extern atomic64_t		ya_count;
extern do_pgfault_func_t	linux_do_pgfault;

asmlinkage void
pgfault_agent(struct pt_regs *pt_regs, unsigned long error_code)
{
#if 0	// Keep it here, later will be used by MM
	unsigned long fault_addr = __save_fault_addr();
#endif
	atomic64_add(PGFAULT_YA_COUNT, &ya_count);

	linux_do_pgfault(pt_regs, error_code);

	atomic64_sub(PGFAULT_YA_COUNT, &ya_count);
}
