/* -*- mode: C; c-file-style: "Linux" -*-
 *
 * Copyright (c) 2018 Luke Huang <lukehuang.ca@gmail.com>
 */

/**
 * \file fork.c
 * \author Luke Huang
 */

#include <linux/sched.h>

#include "sched/sched.h"
#include "user/procfs.h"

extern atomic64_t ya_count;

/* The child */
typedef asmlinkage void (*schedule_tail_func_t)(struct task_struct *);
unsigned long orig_schedule_tail;

#define SCHED_TAIL_YA_COUNT	47111
asmlinkage void
schedule_tail_agent(struct task_struct *prev)
{
	ya_job_t *job;

	atomic64_add(SCHED_TAIL_YA_COUNT, &ya_count);

	((schedule_tail_func_t)orig_schedule_tail)(prev);

	job = search_job_by_task(current);

	/* Deal with thread creation here */
	if (job) {
		/* Only this processs knows it belongs to a certain job,
		 * it needs  to publish  this fact by  initializing some
		 * data and attaching to one thread slot of the job.
		 */
		ya_thread_t *thread = ya_new_thread(job);
                if (thread)
                        add_thread_procfs_entry(thread);
	} /* else: definitely does not  belong to a yasched job */

	atomic64_sub(SCHED_TAIL_YA_COUNT, &ya_count);
}

/* The parent */
typedef asmlinkage long (*do_fork_func_t)(
				unsigned long	 clone_flags,
				unsigned long	 stack_start,
				struct pt_regs	*regs,
				unsigned long	 stack_size,
				int __user	*parent_tidptr,
				int __user	*child_tidptr);

unsigned long orig_do_fork;

#define DO_FORK_YA_COUNT	65719
long
do_fork_agent(unsigned long	 clone_flags,
	      unsigned long	 stack_start,
	      struct pt_regs	*regs,
	      unsigned long	 stack_size,
	      int __user	*parent_tidptr,
	      int __user	*child_tidptr)
{
	long		 ret;
	ya_job_t	*job;

	atomic64_add(DO_FORK_YA_COUNT, &ya_count);

	job = search_job_by_task(current);

	if (job) {
		/* BUG_ON(!thread); // the thread must have been created!
		 */
		get_thread(job);
	}

	ret = ((do_fork_func_t)orig_do_fork)(clone_flags,    stack_start,
					     regs,	     stack_size,
					     parent_tidptr,  child_tidptr);

	if (job && ret <= 0) {
		put_thread(job);
	}

	atomic64_sub(DO_FORK_YA_COUNT, &ya_count);

	return ret;
}
