/* -*- mode: C; c-file-style: "Linux" -*-
 *
 * Copyright (c) 2018 Luke Huang <lukehuang.ca@gmail.com>
 */

/**
 * \file sched.c
 * \author Luke Huang
 */

/* schedule_tail */

/* schedule */

#include <linux/version.h>
#include <linux/sched.h>

#include "lib.h"
#include "kutils.h"

#define SCHEDULE_FUNC_SZ 4096	/* schedule() is pretty big */


static const int	 num_of_nop = 64;
unsigned long  enter_hook;
unsigned long  leave_hook;
unsigned long  jmp_enter_sched_hex;
unsigned char *schedule_start;	/* the point to jump */
unsigned long *schedule_start_p; /* the point to switch */

void			 enter_schedule(void);
void			 leave_schedule(void);
asm (
	"      .text                               	\n"
	"      .section .sched.text,\"ax\"		\n" /* Must be??? */
	"      .globl enter_schedule			\n"
	"  enter_schedule:				\n"
	"      nop; nop; nop; nop; nop; nop; nop; nop;	\n"
	"      nop; nop; nop; nop; nop; nop; nop; nop;	\n"
	"      nop; nop; nop; nop; nop; nop; nop; nop;	\n"
	"      nop; nop; nop; nop; nop; nop; nop; nop;	\n"
	"      nop; nop; nop; nop; nop; nop; nop; nop;	\n"
	"      nop; nop; nop; nop; nop; nop; nop; nop;	\n"
	"      nop; nop; nop; nop; nop; nop; nop; nop;	\n"
	"      nop; nop; nop; nop; nop; nop; nop; nop;	\n"
	"      call	*enter_hook			\n"
	"      jmpq	*schedule_start			\n"
	"      						\n"
	"      .globl leave_schedule			\n"
	"  leave_schedule:				\n"
	"      call	*leave_hook			\n"
	"      pop	%rbx				\n"
	"      pop	%r12				\n"
	"      pop	%r13				\n"
	"      pop	%r14				\n"
	"      pop	%r15				\n"
	"      leaveq					\n"
	"      retq					  "
);

void			 jmp_enter_schedule(void);
asm (
	"      .text                               	\n"
	"      .type jmp_enter_schedule, @function	\n"
	"  jmp_enter_schedule:				\n"
	"      jmp	enter_schedule			  "
);

void			 jmp_leave_schedule(void);
asm (
	"      .text                               	\n"
	"      .type jmp_leave_schedule, @function	\n"
	"  jmp_leave_schedule:				\n"
	"      jmp	leave_schedule			  "
);

static unsigned long __schedule_addr;

int
setup_enter_schedule_hook(unsigned long hook)
{
	unsigned char	*from 	= (unsigned char *)schedule;
	unsigned char	*to	= (unsigned char *)enter_schedule;
	unsigned char	*p;
	int		 i;

        // schedule() calls __schedule()
        p = (typeof(p))memmem(from, 64, "\xe8", 1);
        if (!p)
                goto err;

        __schedule_addr = 0xffffffff00000000 | ((long)p + 5 + *(int *)(p + 1));
        from = (typeof(from))__schedule_addr;
	for (i = 0, p = from; i < num_of_nop; i++, p++) {
                // e.g., 48 83 30	sub $0x30,%rsp
		if (*p == 0x48 && *(p + 1) == 0x83 && *(p + 2) == 0xec) {
                        change_privilege(PAGE_RDWR);
			memcpy(to, from, p - from);
			memcpy(to + (p - from), to + num_of_nop, 14);
                        change_privilege(PAGE_RDONLY);
			break;
		}
	}

        if (*to == 0x90) {
		printk("[%s-%d] could not find signature, abort\n",
		       __FUNCTION__, __LINE__);
                goto err;
	}

	schedule_start = p;
	enter_hook    = hook;
	jmp_enter_sched_hex = *(unsigned long *)&jmp_enter_schedule;
	*(unsigned int *)((unsigned long)&jmp_enter_sched_hex + 1L) =
		(unsigned int)((unsigned long)&enter_schedule -
			       (unsigned long)from - 5UL);

	schedule_start_p    = (unsigned long *)from;
	__ya_xchgq(schedule_start_p, jmp_enter_sched_hex);

	return 0;
err:
        return -ENOSYS;
}

void
remove_enter_schedule_hook(void)
{
	if (likely(schedule_start_p))
		__ya_xchgq(schedule_start_p, jmp_enter_sched_hex);

}

static unsigned long     jmp_leave_sched_hex;
static unsigned long	*schedule_end_p;

/* 5b                      pop    %rbx
 * 41 5c                   pop    %r12
 * 41 5d                   pop    %r13
 * 41 5e                   pop    %r14
 * 41 5f                   pop    %r15
 * 5d	                   pop    %rbp
 * c3                      retq
 */
static const char  sig[] = "\x5b\x41\x5c\x41\x5d\x41\x5e\x41\x5f\x5d\xc3";
int
setup_leave_schedule_hook(unsigned long hook)
{
	schedule_end_p = memmem((unsigned char *)__schedule_addr, SCHEDULE_FUNC_SZ, sig, strlen(sig));
	if (unlikely(!schedule_end_p)) {
		printk("[%s-%d] leave schedule signature not found\n", __FUNCTION__, __LINE__);
		return -ENOSYS;
	}

	jmp_leave_sched_hex = *(unsigned long *)&jmp_leave_schedule;
	*(unsigned int *)((unsigned long)&jmp_leave_sched_hex + 1L) =
		(unsigned int)((unsigned long)&leave_schedule -
			       (unsigned long)schedule_end_p - 5UL);
	leave_hook = hook;

	__ya_xchgq(schedule_end_p, jmp_leave_sched_hex);

	return 0;
}

void
remove_leave_schedule_hook(void)
{
	if (likely(schedule_end_p))
		__ya_xchgq(schedule_end_p, jmp_leave_sched_hex);
}
