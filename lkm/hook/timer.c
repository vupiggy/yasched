/* -*- mode: C; c-file-style: "Linux" -*-
 *
 * Copyright (c) 2018 Luke Huang <lukehuang.ca@gmail.com>
 */

/**
 * \file timer.c
 * \author Luke Huang
 */

#include <linux/kernel.h>
#include <linux/sched.h>

#include <asm/ptrace.h>

#include "sched/sched.h"

typedef void (*apic_timer_interrupt_t)(struct pt_regs *regs);

#define TIMER_YA_COUNT	100801
extern atomic64_t	ya_count;
extern unsigned long	orig_smp_apic_timer_interrupt;

/* WARNING: don't yield the CPU in the interrupt handler!
 */
void
sat_interrupt_agent(struct pt_regs *regs)
{
	struct task_struct	*tsk = current;
	ya_job_t		*job;

	atomic64_add(TIMER_YA_COUNT, &ya_count);

	((apic_timer_interrupt_t)
	 	orig_smp_apic_timer_interrupt)(regs);

	/* As searching the thread  entry needs lock the thread_slot, it
	 * should  try the  best to  avoid using  thread entry  in timer
	 * interrupt handler!
	 */
	job = search_job_by_task(tsk);

	/* NULL job can be because:
	 *
	 * 1. current is not yasched job
	 *
	 * 2. current is  a yasched job but ticked  before going through
	 *    schedule_tail()  thus the intilization  has not  been done
	 *    yet, the best can be done is ignoring this thread now.
	 */
	if (!job)
		goto out;

	/* Let the operation take effect quickly */
	if (job->state == YA_JOB_SUSPENDED ||
	    job->state == YA_JOB_KILLED)
		set_tsk_need_resched(tsk);

out:
	atomic64_sub(TIMER_YA_COUNT, &ya_count);
}

void
schedule_call_agent(void)
{
}
