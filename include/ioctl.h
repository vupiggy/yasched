/* -*- mode: C; c-file-style: "Linux" -*-
 *
 * Copyright (c) 2018 Luke Huang <lukehuang.ca@gmail.com>
 */

/**
 * \file ioctl.h
 * \author Luke Huang
 */

#ifndef _YAS_IOCTL_H_
#define _YAS_IOCTL_H_

#define yacdev_control	"/dev/yacdev0"
#define yacdev_profile	"/dev/yacdev1"

#define YA_IOCTL_MAGIC		'h'

#define YA_IOCTL_SUSPEND_JOB	\
	_IOW(YA_IOCTL_MAGIC, 0, ya_job_op_t)

#define YA_IOCTL_RESUME_JOB	\
	_IOW(YA_IOCTL_MAGIC, 1, ya_job_op_t)

#define YA_IOCTL_KILL_JOB	\
	_IOW(YA_IOCTL_MAGIC, 2, ya_job_op_t)

/* Don't forget to increase this after adding a new ioctl */
#define YA_IOCTL_MAXNR		3

typedef enum {
	KILL_JOB = 0,
	SUSPEND_JOB,
	RESUME_JOB,
	UNKNOWN_CTL
} job_op_t;

typedef struct {
	unsigned long	zone_id;
	unsigned long	job_id;
	job_op_t	op;
} ya_job_op_t;

#ifndef __KERNEL__

int send_ctl(unsigned int req, void *data);

#endif

#endif /* _YAS_IOCTL_H_ */

