/* -*- mode: C; c-file-style: "Linux" -*-
 *
 * Copyright (c) 2018 Luke Huang <lukehuang.ca@gmail.com>
 *
 * $Id$
 */

/**
 * \file relay.h
 * \author Luke Huang
 */

#ifndef _YA_SCHED_PROCFS_H_
#define _YA_SCHED_PROCFS_H_

#include "mgmt/zone.h"
#include "mgmt/job.h"

int	init_procfs(void);
void	fini_procfs(void);

int	add_zone_procfs_entry(ya_zone_t *zone);
int	del_zone_procfs_entry(ya_zone_t *zone);

int	add_job_procfs_entry(ya_job_t *job);
int	del_job_procfs_entry(ya_job_t *job);

int	add_thread_procfs_entry(ya_thread_t *thread);
int	del_thread_procfs_entry(ya_thread_t *thread);

#endif /* _YA_SCHED_PROCFS_H_ */
