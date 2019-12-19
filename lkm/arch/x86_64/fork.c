/* -*- mode: C; c-file-style: "Linux" -*-
 *
 * Copyright (c) 2018 Luke Huang <lukehuang.ca@gmail.com>
 */

/**
 * \file fork.c
 * \author Luke Huang
 */

#include <linux/version.h>
#include <linux/sched.h>

#include <asm/processor.h>
#include <asm/switch_to.h> /* switch_to macro */

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3,11,0))
#  include <linux/kallsyms.h>
#endif

#include "lib.h"
#include "kutils.h"

#define SCHEDULE_FUNC_SZ 4096

/* Values are assigned in syscalls.c */
unsigned long	 stub_clone;
unsigned long	 stub_fork;
unsigned long	 stub_vfork;

/* The child */
extern unsigned long	 orig_schedule_tail;
unsigned long		 schedule_tail_offset;
unsigned int		*schedule_tail_switch_point;

#if 0
/*
 * The idea is that:
 *
 * 1. assembly entry ptregs_sys_fork executes ``lea'' to load address of
 *    sys_fork into rax,  then jmp to stub_ptregs_64, where  rax is used
 *    for another jmp, control path goes into C function.
 *
 * 2. sys_fork()  then calls  _do_fork(), e9  is the pattern to look for
 *
 * 3. _do_fork() calls copy_process(), e8 is the pattern
 *
 * 4. copy_process() calls copy_thread_tls()
 *
 * 5. copy_thread_tls() creates a stack frame with ret_addr set to ret_from_fork
 *
 * 6. assembly entry ret_from_fork() calls schedule_tail
 */
int
switch_schedule_tail(unsigned long agent)
{
        unsigned char *p = (typeof(p))stub_fork;
        unsigned long func;
        // lea sys_fork into rax
        p = (typeof(p))memmem(p, 32, "\x48\x8d\x05", 3);
        if (!p)
                goto err;
        func = 0xffffffff00000000 | ((long)p + 7 + *(int *)(p + 3));

        // _do_fork()
        p = (typeof(p))memmem((typeof(p))func, 64, "\xe9", 1);
        if (!p)
                goto err;
        func = 0xffffffff00000000 | ((long)p + 5 + *(int *)(p + 1));

        // copy_process
        p = (typeof(p))memmem((typeof(p))func, 512, "\xe8", 1);
        if (!p)
                goto err;
        func = 0xffffffff00000000 | ((long)p + 5 + *(int *)(p + 1));

        // copy_thread_tls()



	schedule_tail_switch_point = (unsigned int *)p;
	orig_schedule_tail     = 0xffffffff00000000 |
				 ((unsigned long)p + *(unsigned int *)p + 4);
	schedule_tail_offset   = (unsigned int)
				((unsigned long)agent -
				 (unsigned long)schedule_tail_switch_point - 4);


	__ya_xchgl(schedule_tail_switch_point, schedule_tail_offset);

	return 0;

err:
	return -ENOSYS;
}
#else
int
switch_schedule_tail(unsigned long agent)
{
        unsigned char *p = (typeof(p))kallsyms_lookup_name("ret_from_fork");
        if (!p)
                goto err;

        p = (typeof(p))memmem(p, 64, "\xe8", 1);
        if (!p)
                goto err;

        p++;
	schedule_tail_switch_point = (unsigned int *)p;
	orig_schedule_tail     = 0xffffffff00000000 |
				 ((unsigned long)p + 4 + *(unsigned int *)p);
	schedule_tail_offset   = (unsigned int)
				((unsigned long)agent -
				 (unsigned long)schedule_tail_switch_point - 4);

	__ya_xchgl(schedule_tail_switch_point, schedule_tail_offset);

	return 0;
err:
        return -ENOSYS;
}
#endif

void
restore_schedule_tail(void)
{
	if (likely(schedule_tail_switch_point))
		__ya_xchgl(schedule_tail_switch_point, schedule_tail_offset);
}

/* The parent */
extern unsigned long	orig_do_fork;
static inline unsigned long
search_do_fork(void)
{
#ifdef CONFIG_KALLSYMS
	/* This can be 100% sure */
	return get_symbol_from_kallsyms("_do_fork");
#else
	return NULL;
#endif
}

#define search_sys_fork_family(f)						\
({										\
        unsigned long addr;                                             	\
        addr = 0xffffffff00000000 | (stub_##f + 7 + *(int *)(stub_##f + 3));	\
})

#define __switch_do_fork(f)						\
({									\
	int distance = 0;						\
	unsigned char *p;						\
	unsigned long do_fork_addr;					\
									\
	p = (__typeof__(p))sys_##f;					\
									\
	while (distance < 128) {					\
		p = memmem((unsigned char *)sys_##f, 128 - distance,	\
			   "\xe9", 1);					\
		if (!p) p = memmem((unsigned char *)sys_##f,		\
				   128 - distance, "\xe8", 1);		\
		if (unlikely(!p)) {					\
			printk("[%s-%d] %p\n", __FUNCTION__, __LINE__,	\
			       (void *)sys_##f);			\
			restore_do_fork();				\
			goto err;					\
		}							\
									\
		p++;							\
		distance += (unsigned long)(p - sys_##f);		\
		do_fork_addr =						\
		((unsigned long)(p + 4 + *(unsigned int *)(p)) & 	\
		 0x00000000ffffffff) | 0xffffffff00000000;		\
									\
		if (do_fork_addr == orig_do_fork ||			\
		    (!orig_do_fork &&					\
		     (((unsigned char *)do_fork_addr)[0] == 0x55 ||	\
		      (((unsigned char *)do_fork_addr)[0] == 0x41 &&	\
		       ((unsigned char *)do_fork_addr)[1] == 0x57)))) {	\
			f##_do_fork_switch_point = (unsigned int *)p;	\
			f##_do_fork_offset =				\
			(unsigned int)(agent -				\
		        (unsigned long)f##_do_fork_switch_point - 4); 	\
									\
			__ya_xchgl(f##_do_fork_switch_point,		\
				   f##_do_fork_offset);			\
			break;						\
		}							\
	}								\
})

unsigned long	 clone_do_fork_offset;
unsigned int	*clone_do_fork_switch_point;
unsigned long	 fork_do_fork_offset;
unsigned int	*fork_do_fork_switch_point;
unsigned long	 vfork_do_fork_offset;
unsigned int	*vfork_do_fork_switch_point;

void
restore_do_fork(void)
{
	if (likely(clone_do_fork_switch_point))
		__ya_xchgl(clone_do_fork_switch_point, clone_do_fork_offset);

	if (likely(fork_do_fork_switch_point))
		__ya_xchgl(fork_do_fork_switch_point, fork_do_fork_offset);

	if (likely(vfork_do_fork_switch_point))
		__ya_xchgl(vfork_do_fork_switch_point, vfork_do_fork_offset);
}

int
switch_do_fork(unsigned long agent)
{
	unsigned long	sys_clone;
	unsigned long	sys_fork;
	unsigned long	sys_vfork;

	sys_clone = search_sys_fork_family(clone);
	sys_fork  = search_sys_fork_family(fork);
	sys_vfork = search_sys_fork_family(vfork);

	if (unlikely(!sys_clone || !sys_fork || !sys_vfork))
		goto err;

	orig_do_fork = search_do_fork();

	__switch_do_fork(clone);
	__switch_do_fork(fork);
	__switch_do_fork(vfork);

	return 0;

err:
	return -ENOSYS;
}
