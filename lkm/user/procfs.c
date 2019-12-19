/* -*- mode: C; c-file-style: "Linux" -*-
 *
 * Copyright (c) 2018 Luke Huang <lukehuang.ca@gmail.com>
 *
 * $Id$
 */

/**
 * \file relay.c
 * \author Luke Huang
 */

#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/stat.h>

#include "mgmt/zone.h"
#include "mgmt/job.h"
#include "sched/sched_types.h"
#include "procfs.h"

#define	YA_PROCFS_ROOTDIR	"yasched"
static struct proc_dir_entry *ya_procfs_rootdir;

/* root */
int
init_procfs(void)
{
	ya_procfs_rootdir = proc_mkdir(YA_PROCFS_ROOTDIR, NULL);
	if (!ya_procfs_rootdir)
		return -ENOMEM;

	return 0;
}

void
fini_procfs(void)
{
	remove_proc_entry(YA_PROCFS_ROOTDIR, NULL);
}

/* Zone */
int
add_zone_procfs_entry(ya_zone_t *zone)
{
	/* Don't allow zone_id > 9999999, i.e, 7 characters */
	if (zone->zone_id > 9999999UL)
		return -EFAULT;

	snprintf(zone->procfs_dir_name, sizeof(zone->procfs_dir_name) - 1,
		 "zone-%lu", zone->zone_id);
	zone->procfs_dir_entry = proc_mkdir(zone->procfs_dir_name,
					    ya_procfs_rootdir);

	if (!unlikely(zone->procfs_dir_entry))
		return -ENOMEM;

	return 0;
}

int
del_zone_procfs_entry(ya_zone_t *zone)
{
	if (likely(zone->procfs_dir_entry))
		remove_proc_entry(zone->procfs_dir_name, ya_procfs_rootdir);

	return 0;
}

/* Job */
int
add_job_procfs_entry(ya_job_t *job)
{
	if (job->id > (1UL << 59))
		return -EFAULT;

	snprintf(job->procfs_dir_name, sizeof(job->procfs_dir_name) - 1,
		 "job-%lu", job->id);
	job->procfs_dir_entry = proc_mkdir(job->procfs_dir_name,
					   job->attribute.zone->procfs_dir_entry);
	if (unlikely(job->procfs_dir_entry))
		return -ENOMEM;

	return 0;
}

int
del_job_procfs_entry(ya_job_t *job)
{
	if (likely(job->procfs_dir_entry))
		remove_proc_entry(job->procfs_dir_name,
				  job->attribute.zone->procfs_dir_entry);

	return 0;
}

/* thread */
#include "procfs_thread.c"

int
add_thread_procfs_entry(ya_thread_t *thread)
{
	snprintf(thread->procfs_entry_name, sizeof(thread->procfs_entry_name) - 1,
		 "%d", thread->task->pid);

	thread->procfs_entry = proc_create_data(thread->procfs_entry_name,
                                                S_IFREG | S_IRUGO,
                                                thread->job->procfs_dir_entry,
                                                &thread_procfs_ops, thread);

	if (unlikely(!thread->procfs_entry))
		return -ENOMEM;

	return 0;
}

int
del_thread_procfs_entry(ya_thread_t *thread)
{
	if (likely(thread->procfs_entry)) {
		remove_proc_entry(thread->procfs_entry_name,
				  thread->job->procfs_dir_entry);
        }

	return 0;
}
