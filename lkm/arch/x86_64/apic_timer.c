/* -*- mode: C; c-file-style: "Linux" -*-
 *
 * Copyright (c) 2018 Luke Huang <lukehuang.ca@gmail.com>
 */

/**
 * \file apic_timer.c
 * \author Luke Huang
 */

#include <linux/version.h>
#include <linux/kernel.h>

#include <asm/ptrace.h>
#include <asm/hw_irq.h>

#include "lib.h"
#include "kutils.h"

extern unsigned long irq_vector_entry(int v);

unsigned int	*timer_switch_point;
unsigned int	 timer_offset;
unsigned long	 orig_smp_apic_timer_interrupt;

int
switch_smp_apic_timer_interrupt(unsigned long agent)
{
	unsigned long	 sat_interrupt_entry;
	unsigned char	*p;

	sat_interrupt_entry = irq_vector_entry(LOCAL_TIMER_VECTOR);

#ifdef INCEPTION_DEBUG
	printk("[%s-%d] apic_timer_interrupt: %p\n", __FUNCTION__, __LINE__, (void *)sat_interrupt_entry);
#endif

	if (unlikely(!sat_interrupt_entry))
		goto no_sys;


        // switch_to_thread_stack
	p = (unsigned char*)memmem((void *)sat_interrupt_entry,
				   MAX_OFFSET, "\xe8", 1);
	if (unlikely(!p)) {
		printk("Couldn't find 1st ``callq'' in apic "
		       "timer interrupt handler. Abort\n");
		goto no_sys;
	}

        // enter_from_user_mode
	p = (unsigned char *)memmem(p + 1, MAX_OFFSET, "\xe8", 1);
	if (unlikely(!p)) {
		printk("Couldn't find the 2nd ``callq'' in apic "
		       "timer interrupt handler. Abort\n");
		goto no_sys;
	}

        // smp_apic_timer_interrupt
	p = (unsigned char *)memmem(p + 1, MAX_OFFSET, "\xe8", 1);
	if (unlikely(!p)) {
		printk("Couldn't find the 2nd ``callq'' in apic "
		       "timer interrupt handler. Abort\n");
		goto no_sys;
	}

	timer_switch_point = (unsigned int *)(p + 1);
	timer_offset	   = (unsigned int)(agent -
				     (unsigned long)timer_switch_point - 4);
	orig_smp_apic_timer_interrupt = 0xffffffff00000000 | (unsigned long)
		(*timer_switch_point + (unsigned long)timer_switch_point + 4);

	if (!verify_symbol(orig_smp_apic_timer_interrupt,
			   "smp_apic_timer_interrupt")) {
		goto no_sys;
	}

#ifdef INCEPTION_DEBUG
	printk("[%s-%d] smp_apic_timer_interrupt: %p\n",
	       __FUNCTION__, __LINE__, (void *)orig_smp_apic_timer_interrupt);

	timer_switch_point = 0;
#else
	__ya_xchgl(timer_switch_point, timer_offset);
#endif

	return 0;

no_sys:
	return -ENOSYS;
}

void
restore_smp_apic_timer_interrupt(void)
{
	if (likely(timer_switch_point))
		__ya_xchgl(timer_switch_point, timer_offset);
}

int
switch_schedule_call(unsigned long agent)
{
	return 0;
}

void
restore_schedule_call(void)
{
}
