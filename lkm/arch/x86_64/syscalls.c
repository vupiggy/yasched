/* -*- mode: C; c-file-style: "Linux" -*-
 *
 * Copyright (c) 2018 Luke Huang <lukehuang.ca@gmail.com>
 */

/**
 * \file syscalls.c
 * \author Luke Huang
 */

#include <linux/version.h>
#include <linux/module.h>
#include <linux/fs.h>		/* getname */

// xxx luke xxx
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3,11,0))
#  include <linux/kallsyms.h>
#endif

#include <asm/unistd.h>		/* __NR_XXX */
#include <asm/processor.h>	/* pt_regs */

#include "lib.h"
#include "kutils.h"
#include "syscalls.h"

#define MAX_OFFSET 512

unsigned long	 *linux_syscall_table;
asmlinkage int   (*linux_sys_modify_ldt)(int		 func,
					 void __user	*ptr,
					 unsigned long	 bytecount);
extern unsigned long	stub_clone;
extern unsigned long	stub_fork;
extern unsigned long	stub_vfork;

// xxx luke xxx
// TODO: from which version we can not get correct syscall entry from MSR
#if (LINUX_VERSION_CODE < KERNEL_VERSION(3,11,0))
static unsigned long
syscall_from_msr(void)
{
        unsigned long	 system_call = 0;
	unsigned char	*p;
	unsigned long	 tmp;
        rdmsrl(MSR_LSTAR, system_call);

	/* we assume rdmsr won't fail */
	p = (unsigned char *)memmem((void *)system_call, MAX_OFFSET,
				    "\xff\x14\xc5", 3);
	if (unlikely(!p)) {
		printk("[%s-%d] Couldn't find the system call table from %p\n",
		       __FUNCTION__, __LINE__, (void *)system_call);

		return 0;
	}

	tmp = *(unsigned long *)(p + 3);
        return ((tmp & 0x00000000ffffffff) | 0xffffffff00000000);
}
#else
static unsigned long
syscall_from_kallsyms(void)
{
        return kallsyms_lookup_name("sys_call_table");
}
#endif

static unsigned long
search_syscall_table_address(void)
{
        // TODO: from which version this hack stops working? xxx luke xxx
#if (LINUX_VERSION_CODE < KERNEL_VERSION(3,11,0))
        return syscall_from_msr();
#else
        return syscall_from_kallsyms();
#endif
}

unsigned int		*do_execve_switch_point;
unsigned int		 do_execve_offset;
extern unsigned long	 orig_do_execve;
// TODO: find a cleaner/better way! xxx luke xxx
static int
switch_execve(unsigned long agent)
{
        int ret = -ENOSYS;
        unsigned char *p = (typeof(p))kallsyms_lookup_name("SyS_execve");
        if (p) {
                unsigned char *call_ptr;
                unsigned long func;
                unsigned long getname_addr = kallsyms_lookup_name("getname");
                call_ptr = memmem(p, 64, "\xe8", 1);

                if (unlikely(!call_ptr)) {
			printk("[%s-%d] ``call 1'' not found!\n", __FUNCTION__, __LINE__);
                        goto out;
		}
                func = 0xffffffff00000000 | ((unsigned long)call_ptr + 5 +
                                             *(unsigned int *)(call_ptr + 1));
                if (func != getname_addr) {
                        printk(KERN_WARNING "[%s-%d] call getname not found\n",
                               __FUNCTION__, __LINE__);
                        goto out;
                }
                call_ptr += 5;
                call_ptr = memmem(call_ptr, 64, "\xe8", 1);
                if (unlikely(!call_ptr)) {
			printk("[%s-%d] ``call 2'' not found!\n", __FUNCTION__, __LINE__);
                        goto out;
		}
                func = 0xffffffff00000000 | ((unsigned long)call_ptr + 5 +
                                             *(unsigned int *)(call_ptr + 1));
                orig_do_execve = func;
                do_execve_switch_point = (unsigned int *)(call_ptr + 1);
                do_execve_offset       = (unsigned int)(agent - (unsigned long)do_execve_switch_point - 4);
                __ya_xchgl(do_execve_switch_point, do_execve_offset);

                ret = 0;
        }

out:
        return ret;
}

static void
restore_execve(void)
{
	if (likely(do_execve_switch_point))
		__ya_xchgl(do_execve_switch_point, do_execve_offset);
}

extern asmlinkage long
execve_agent(int fd, struct filename *filename,
             struct user_arg_ptr argv,
             struct user_arg_ptr envp,
             int flags);

int
switch_syscalls(void)
{
	int ret = -ENOSYS;

	linux_syscall_table = (unsigned long *)search_syscall_table_address();
        if (!linux_syscall_table) {
                printk(KERN_WARNING "Point %d found\n", PT_SYSCALL);
                goto out;
        }

	if ((ret = switch_execve((unsigned long)execve_agent)) != 0)
		goto out;

	linux_sys_modify_ldt = (void *)linux_syscall_table[__NR_modify_ldt];
	stub_clone = linux_syscall_table[__NR_clone];
	stub_fork  = linux_syscall_table[__NR_fork];
	stub_vfork = linux_syscall_table[__NR_vfork];
        ret = 0;
out:
	return ret;
}

void
restore_syscalls(void)
{
	restore_execve();
}
