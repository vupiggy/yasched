/* -*- mode: C; c-file-style: "Linux" -*-
 *
 * Copyright (c) 2018 Luke Huang <lukehuang.ca@gmail.com>
 */

/**
 * \file page_fault.c
 * \author Luke Huang
 */

#include <linux/version.h>
#include <linux/module.h>
#include <linux/linkage.h>

#include <asm/ptrace.h>

#include "lib.h"
#include "kutils.h"

#define PGFAULT_MODIFIER asmlinkage
typedef PGFAULT_MODIFIER void (*do_pgfault_func_t)(struct pt_regs *,
                                                   unsigned long);
do_pgfault_func_t		 linux_do_pgfault;
unsigned long			 error_entry;
unsigned int			*switch_point;
unsigned int			 offset;

#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,29))
do_pgfault_func_t		 ya_do_pgfault;
asmlinkage void error_entry_agent(void);
asm (
        "      .text                               \n"
        "      .type error_entry_agent, @function  \n"
        "  error_entry_agent:                      \n"
        "      movq %rax, linux_do_pgfault         \n"
        "      movq ya_do_pgfault, %rax            \n"
        "      jmpq *error_entry                     "
);
#endif

extern unsigned long irq_vector_entry(int v);

int
switch_pgfault_handler(unsigned long agent)
{
	unsigned long    pgfault_entry;
	unsigned char	*p;

        /* check arch/x86_64/kernel/traps.c */
        pgfault_entry = irq_vector_entry(14);
        if (unlikely(!pgfault_entry)) {
                printk("Couldn't find the error entry "
                       "for page fault. Abort\n");
                goto no_sys;
        }

#ifdef INCEPTION_DEBUG
        printk("[%s-%d]: page_fault error entry: %p\n",
	       __FUNCTION__, __LINE__,
	       (void *)pgfault_entry);
#endif

        // call error_entry
	p = (unsigned char *)memmem((unsigned char *)pgfault_entry,
				    MAX_OFFSET, "\xe8", 1);
        // call do_page_fault
	p = (unsigned char *)memmem((unsigned char *)(p + 1),
				    MAX_OFFSET, "\xe8", 1);
	switch_point	 = (unsigned int *)(p + 1);
	offset		 = (unsigned int)((unsigned long)agent -
					  (unsigned long)p - 5);
	linux_do_pgfault = (void *)(0xffffffff00000000 |
				    (*switch_point + (unsigned long)p + 5));

#ifdef INCEPTION_DEBUG
	printk("[%s-%d] error_entry: %p do_pgfault: %p\n",
	       __FUNCTION__, __LINE__,
	       (void *)error_entry, linux_do_pgfault);
	switch_point = 0;
#else
	__ya_xchgl(switch_point, offset);
#endif

        return 0;

no_sys:
	return -ENOSYS;
}

void
restore_pgfault_handler(void)
{
	if (likely(switch_point))
		__ya_xchgl(switch_point, offset);
}
