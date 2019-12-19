/* -*- mode: C; c-file-style: "Linux" -*-
 *
 * Copyright (c) 2018 Luke Huang <lukehuang.ca@gmail.com>
 */

/**
 * \file sched.h
 * \author Luke Huang
 */

#ifndef _YAS_SCHED_H_
#define _YAS_SCHED_H_

#include <linux/list.h>
#include <linux/sched.h>
#include <linux/sched/signal.h>

#include <asm/atomic.h>
#include <asm/mmu_context.h>

#include "mgmt/job.h"
#include "mgmt/zone.h"
#include "sched_types.h"

#define PF_YA_JOB		PF_ALIGNWARN /* steal this unused flag */
#define set_tsk_ya_job(tsk)	(tsk)->flags |= PF_YA_JOB
#define clr_tsk_ya_job(tsk)	(tsk)->flags &= ~PF_YA_JOB
#define tst_tsk_ya_job(tsk)	((tsk)->flags & PF_YA_JOB)

int	init_sched(void);
void	exit_sched(void);

// WTF is it? xxx luke xxx
extern ya_zone_t ya_sched_zone;
static inline ya_zone_t *
__search_zone_by_id(zid_t zid)
{
	return (zid == 0) ? &ya_sched_zone : NULL;
}

ya_job_t	*ya_new_job(ya_job_attr_t *job_attr);
void		 ya_del_job(ya_job_t *job);
int		 kill_job(zid_t zid, jid_t jid);
int		 suspend_job(zid_t zid, jid_t jid);
int		 resume_job(zid_t zid, jid_t jid);

static inline void
get_thread(ya_job_t *job) {
	atomic_inc(&job->alive_proc);
}

static inline void
put_thread(ya_job_t *job)
{
	if (atomic_dec_and_test(&job->alive_proc))
		ya_del_job(job);
}

ya_thread_t	*ya_new_thread(ya_job_t *job);
void		 ya_del_thread(ya_thread_t *thread);

static inline void
go_to_bed(ya_thread_t *thread, ya_job_t *job)
{
	prepare_to_wait(&job->suspended_threads,
			&thread->suspend_wait, TASK_INTERRUPTIBLE);
}

static inline int
thread_suspended(ya_thread_t *thread)
{
	return !list_empty(&thread->suspend_wait.entry);
}

static inline void
get_up(ya_thread_t *thread, ya_job_t *job)
{
	finish_wait(&job->suspended_threads, &thread->suspend_wait);
	/* finish_wait() set the task state as TASK_RUNNING */
	thread->task->state = thread->task_state;
}

static inline void
go_to_heaven(ya_thread_t *thread)
{
	/* Heaven is good but not allowed to enter twice */
	if (!thread || thread->state == YA_THREAD_DEAD)
		return;

	thread->task->state = thread->task_state;
	thread->state = YA_THREAD_DEAD;
	kill_pid(task_pid(thread->task), SIGKILL, 1);
}

#endif /* _YAS_SCHED_H_ */
