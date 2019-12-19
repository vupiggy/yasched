/* -*- mode: C; c-file-style: "Linux" -*-
 *
 * Copyright (c) 2018 Luke Huang <lukehuang.ca@gmail.com>
 */

/**
 * \file sched.c
 * \author Luke Huang
 */

#include <linux/kernel.h>
#include <linux/linkage.h>
#include <linux/sched.h>
#include <linux/sched/debug.h>

#include "sched/sched.h"

extern atomic64_t ya_count;

#define ENTER_SCHED_YA_COUNT	104711

void __sched
enter_schedule_hook(void)
{
	struct task_struct	*tsk	= current;
	ya_job_t		*job;
	ya_thread_t		*thread;

	atomic64_add(ENTER_SCHED_YA_COUNT, &ya_count);

	thread = search_thread_by_task(tsk);

	/* NULL thread can be because:
	 *
	 * 1. current is not  a yasched job
	 *
	 * 2. current is a yasched  job, but schedule() gets call during
	 *    the creation of a  new thread, e.g, kmem_cache_alloc calls
	 *    cond_schedule()  to  allocate the  thread  entry. This  is
	 *    possible but rarely happening.
	 */
	if (!thread)
		goto out;

	if (tsk->state == TASK_DEAD) {
		thread->state = YA_THREAD_DEAD;
		goto out;
	}

	job = thread->job;
	thread->task_state = thread->task->state;
	go_to_bed(thread, job);

	switch(job->state) {
	case YA_JOB_SUSPENDED:
		break;
	case YA_JOB_KILLED:
		get_up(thread, job);
		go_to_heaven(thread);
		break;
	default:
		get_up(thread, job);
		break;
	}

out:
	atomic64_sub(ENTER_SCHED_YA_COUNT, &ya_count);
}

#define LEAVE_SCHED_YA_COUNT	102233
void __sched
leave_schedule_hook(void)
{
	struct task_struct	*tsk = current;
	ya_thread_t		*thread;
	ya_job_t		*job;

	atomic64_add(LEAVE_SCHED_YA_COUNT, &ya_count);

	thread = search_thread_by_task(tsk);
	if (!thread)	/* same reason as enter_schedeule */
		goto out;

	job = thread->job;
	if (thread_suspended(thread))
		get_up(thread, job);

out:
	atomic64_sub(LEAVE_SCHED_YA_COUNT, &ya_count);
}
