/* -*- mode: C; c-file-style: "Linux" -*-
 *
 * Copyright (c) 2018 Luke Huang <lukehuang.ca@gmail.com>
 */

/**
 * \file ioctl.c
 * \author Luke Huang
 */

#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>

#include <asm/uaccess.h>

#include "sched/sched.h"
#include "ioctl.h"

const char *job_op_str[] = {
	"SUSPEND",
	"RESUME",
	"KILL",
	"UNKNOWN"
};
typedef int (*job_op_func_t)(zid_t zid, jid_t jid);
const job_op_func_t job_op_funcs[] = {
	kill_job,
	suspend_job,
	resume_job,
	NULL
};

/* For simplicity, parallel  ioctl is not allowed. We ass  u me that the
 * admin/control operation won't happen frequently.
 */
DEFINE_MUTEX(ya_ioctl_mutex);

static int
__handle_job_op(unsigned long arg)
{
	ya_job_op_t	 job_op;

	if (copy_from_user(&job_op, (ya_job_op_t *)arg, sizeof(job_op)))
		return -EFAULT;

	if (job_op.op < KILL_JOB || job_op.op >= UNKNOWN_CTL)
		return -EINVAL;

	return job_op_funcs[job_op.op](job_op.zone_id, job_op.job_id);
}

long
yacdev_ioctl(struct file	*filp,
	     unsigned int	 cmd,
	     unsigned long	 arg)
{
	long err = 0;

	if (_IOC_TYPE(cmd) != YA_IOCTL_MAGIC)
		return -ENOTTY;

	if (_IOC_DIR(cmd) & _IOC_READ)
		err = !access_ok(VERIFY_WRITE,
				 (void __user *)arg, _IOC_SIZE(cmd));
	else if (_IOC_DIR(cmd) & _IOC_WRITE)
		err = !access_ok(VERIFY_READ,
				 (void __user *)arg, _IOC_SIZE(cmd));
	if (err) return -EFAULT;

	switch(cmd) {
	case YA_IOCTL_KILL_JOB:
	case YA_IOCTL_SUSPEND_JOB:
	case YA_IOCTL_RESUME_JOB:
		mutex_lock(&ya_ioctl_mutex);
		err = __handle_job_op(arg);
		mutex_unlock(&ya_ioctl_mutex);

		break;
	default:
		return -ENOTTY;
	};

	return err;
}
