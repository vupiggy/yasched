/* -*- mode: C; c-file-style: "Linux" -*-
 *
 * Copyright (c) 2018 Luke Huang <lukehuang.ca@gmail.com>
 */

/**
 * \file sched.c
 * \author Luke Huang
 */

#include <linux/sched.h>
#include <linux/sched/debug.h>
#include <asm/string.h>

#include "sched.h"
#include "user/procfs.h"

/* zone */

/* TODO: zones should be able to dynamically create and destory!!! */
ya_zone_t ya_sched_zone = { .zone_id = 0 };

static struct kmem_cache *job_cache;
static struct kmem_cache *thread_cache;
const char               *job_cache_name    = "ya_jache";
const char               *thread_cache_name = "ya_tache";

#define init_job_id(z,val)	((z)->max_jid = (val))
#define inc_jid_return(z)	(++(z)->max_jid)

#define init_ya_zone_lock(z)	spin_lock_init(&(z)->lock)
#define lock_ya_zone(z)		spin_lock(&(z)->lock);
#define unlock_ya_zone(z)	spin_unlock(&(z)->lock);

static inline void
__init_zone(ya_zone_t *z)
{
	init_ya_zone_lock(z);
	INIT_LIST_HEAD(&z->job_list);
	init_job_id(z, -1);
	add_zone_procfs_entry(z);

	printk("[YASched]: Zone is initialized\n");
}

static inline void
__exit_zone(ya_zone_t *z)
{
	del_zone_procfs_entry(z);
}

/* thread */
#define ALLOC_TRIES	9

// TODO: xxx luke xxx
// This is wrong, ``current'' might have already gone when we cat this file
static inline void
__init_thread(ya_thread_t *thread)
{
	thread->task  = current;
	thread->state = YA_THREAD_ALIVE;
        INIT_LIST_HEAD(&thread->list);
	init_wait(&thread->suspend_wait);
}

static inline void
__attach_to_job(ya_thread_t *thread, ya_job_t *job)
{
	ya_thread_slot_t *thread_slot;

	thread->job = job;
	thread_slot = &thread->job->thread_table[
				thread->task->pid % THREAD_TABLE_SIZE];
	write_lock(&thread_slot->lock);
        if (list_empty(&thread->list))
                list_add_tail(&thread->list, &thread_slot->list);
	write_unlock(&thread_slot->lock);
}

static int
__ya_init_thread(ya_thread_t *thread, ya_job_t *job)
{
	__init_thread(thread);
	__attach_to_job(thread, job);

	return 0;
}

ya_thread_t *
ya_new_thread(ya_job_t *job)
{
	ya_thread_t	*thread;
	int		 tries = 0;

	while(tries++ < ALLOC_TRIES) {
		thread = kmem_cache_alloc(thread_cache, GFP_KERNEL);
		if (likely(thread))
                        break;

		set_current_state(TASK_INTERRUPTIBLE);
		schedule_timeout(HZ >> 3);
	}

	if (unlikely(!thread)) return NULL;

	__ya_init_thread(thread, job);
	return thread;
}

static inline void
__fini_thread(ya_thread_t *thread)
{
	ya_job_t *job = thread->job;

	del_thread_procfs_entry(thread);

	kmem_cache_free(thread_cache, thread);

	/* Dec the threads count, if it is down to zero then the job can
	 * be detroyed safely.
	 */
	put_thread(job);
}

static inline void
__detach_from_job(ya_thread_t *thread)
{
	ya_thread_slot_t	*thread_slot;
	ya_job_t		*job = thread->job;

	thread_slot = &job->thread_table[current->pid % THREAD_TABLE_SIZE];

	write_lock(&thread_slot->lock);
	list_del_init(&thread->list);
	write_unlock(&thread_slot->lock);
}

void
ya_del_thread(ya_thread_t *thread)
{
	__detach_from_job(thread);
	__fini_thread(thread);
}

/* job */
static ya_job_t *
__ya_init_job(ya_job_t *job, ya_job_attr_t *job_attr)
{
	int			 i;
	ya_thread_slot_t	*thread_slot;
	ya_zone_t		*zone;
	ya_thread_t		*thread;

	for (i = 0; i < THREAD_TABLE_SIZE; ++i) {
		thread_slot = &job->thread_table[i];
		rwlock_init(&thread_slot->lock);
		INIT_LIST_HEAD(&thread_slot->list);
	}

	if ((thread = ya_new_thread(job)) == NULL) {
		kmem_cache_free(job_cache, job);
		return NULL;
	}

	memcpy(&job->attribute, job_attr, sizeof(*job_attr));

	job->state = YA_JOB_RUNNING;
	atomic_set(&job->alive_proc, 1);
	init_waitqueue_head(&job->suspended_threads);

	zone = job_attr->zone;
	BUG_ON(!zone);

	lock_ya_zone(zone);

	job->attribute.zone = zone;
	job->id             = inc_jid_return(zone);
	list_add_tail(&job->list, &zone->job_list);

	unlock_ya_zone(zone);

	add_job_procfs_entry(job);
	add_thread_procfs_entry(thread);

	printk("[YASched]: Job %lu entered\n", job->id);

	return job;
}

ya_job_t *
ya_new_job(ya_job_attr_t *job_attr)
{
	ya_job_t		*job;
	int			 tries = 0;

	while(tries++ < ALLOC_TRIES) {
		job = kmem_cache_alloc(job_cache, GFP_KERNEL);
		if (likely(job))
                        break;

		set_current_state(TASK_INTERRUPTIBLE);
		schedule_timeout(HZ >> 3);
	}

	if (unlikely(!job))
                return NULL;

	return __ya_init_job(job, job_attr);
}

static inline int
__kill_job(ya_job_t *job)
{
	int prev_state = job->state;

	printk("[YASched]: kill job %lu\n", job->id);

	job->state = YA_JOB_KILLED;
	wmb();

	if (prev_state == YA_JOB_SUSPENDED)
		wake_up(&job->suspended_threads);

	return 0;
}

static inline int
__suspend_job(ya_job_t *job)
{
	printk("[YASched]: suspend job %lu\n", job->id);
	job->state = YA_JOB_SUSPENDED;
	return 0;
}

static int
__resume_job(ya_job_t *job)
{
	printk("[YASched]: resume job %lu\n", job->id);
	if (job->state == YA_JOB_SUSPENDED) {
		job->state = YA_JOB_RUNNING;
		wmb();
		wake_up(&job->suspended_threads);
	}

	return 0;
}

/* Call this function with zone's lock holding */
static ya_job_t *
search_job_by_id(ya_zone_t *zone, jid_t jid)
{
	ya_job_t *job;

	for_each_job_in_zone(zone,job) {
		if (job->id == jid)
			return job;
	}

	return NULL;
}

int
kill_job(zid_t zid, jid_t jid)
{
	ya_job_t	*job;
	ya_zone_t	*zone;
	int		 ret;

	if (!(zone = __search_zone_by_id(zid)))
		return -EINVAL;

	lock_ya_zone(zone);

	job = search_job_by_id(zone, jid);
	if (job)
		ret = __kill_job(job);
	else
		ret = -EINVAL;

	unlock_ya_zone(zone);

	return ret;
}

int
suspend_job(zid_t zid, jid_t jid)
{
	ya_job_t	*job;
	ya_zone_t	*zone;
	int		 ret;

	if (!(zone = __search_zone_by_id(zid)))
		return -EINVAL;

	lock_ya_zone(zone);

	job = search_job_by_id(zone, jid);
	if (job)
		ret = __suspend_job(job);
	else
		ret = -EINVAL;

	unlock_ya_zone(zone);

	return ret;
}

int
resume_job(zid_t zid, jid_t jid)
{
	ya_job_t	*job;
	ya_zone_t	*zone;
	int		 ret;

	if (!(zone = __search_zone_by_id(zid)))
		return -EINVAL;

	lock_ya_zone(zone);

	job = search_job_by_id(zone, jid);
	if (job)
		ret = __resume_job(job);
	else
		ret = -EINVAL;

	unlock_ya_zone(zone);

	return ret;
}

void
ya_del_job(ya_job_t *job)
{
	ya_zone_t *zone = job->attribute.zone;

        ya_job_t *dummy_job = NULL;

	lock_ya_zone(zone);
	list_del(&job->list);
	unlock_ya_zone(zone);

        del_job_procfs_entry(job);
        kmem_cache_free(job_cache, job);

	printk("[YASched]: Job %lu left %lu\n", job->id, dummy_job->id);
}


int __init
init_sched(void)
{
	int	ret = -ENOMEM;

	__init_zone(&ya_sched_zone);

	job_cache = kmem_cache_create(job_cache_name, sizeof(ya_job_t), 0, 0, NULL);
	if (unlikely(!job_cache)) {
		printk("[%s-%d]: failed in creating the jobs cache\n",
		       __FUNCTION__, __LINE__);
		goto err;
	}

	thread_cache = kmem_cache_create(thread_cache_name, sizeof(ya_thread_t), 0, 0, NULL);
	if (unlikely(!thread_cache)) {
		printk("[%s-%d]: failed in creating the threads cache\n",
		       __FUNCTION__, __LINE__);
		goto err;
	}

	return 0;

err:
	return ret;
}

void
exit_sched(void)
{

	__exit_zone(&ya_sched_zone);

	if (job_cache)
		kmem_cache_destroy(job_cache);

	if (thread_cache)
		kmem_cache_destroy(thread_cache);
}
