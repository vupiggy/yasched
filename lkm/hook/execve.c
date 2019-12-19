/* -*- mode: C; c-file-style: "Linux" -*-
 *
 * Copyright (c) 2018 Luke Huang <lukehuang.ca@gmail.com>
 */

/**
 * \file execve.c
 * \author Luke Huang
 */

#include <linux/kernel.h>
#include <linux/linkage.h>
#include <linux/sched.h>
#include <linux/mm.h>

#include <asm/atomic.h>
#include <asm/uaccess.h>
#include <asm/mman.h>

#include "kutils.h"
#include "sched/sched.h"

#define EXECVE_YA_COUNT	100333
extern atomic64_t	ya_count;

typedef asmlinkage long (*execve_func_t)(
        int fd, struct filename *filename,
        struct user_arg_ptr argv,
        struct user_arg_ptr envp,
        int flags);
unsigned long		  orig_do_execve;

enum job_status {
        NON_JOB = -1,
        NEW_JOB,
        EXISTING_JOB
};

const unsigned long *ya_zone_str = (unsigned long *)"YA_SCHED";
const unsigned long *zone_id_str = (unsigned long *)"ZONE_ID=";

static inline long
my_atol(const char *name)
{
	char *s = (char *)name;
	int val = 0;

	for (; (name - s) < 20; name++) {
		switch (*name) {
		case '0'...'9':
			val = 10*val+(*name-'0');
			break;
		default:
			return val;
		}
	}

	return val;
}

#define ZONE_ID_SET			1
#define JOB_ATTR_ALL_SET		(ZONE_ID_SET)

static const char __user *get_user_arg_ptr(struct user_arg_ptr argv, int nr)
{
        const char __user *native;

#ifdef CONFIG_COMPAT
        if (unlikely(argv.is_compat)) {
                compat_uptr_t compat;

                if (get_user(compat, argv.ptr.compat + nr))
                        return ERR_PTR(-EFAULT);

                return compat_ptr(compat);
        }
#endif

        if (get_user(native, argv.ptr.native + nr))
                return ERR_PTR(-EFAULT);

        return native;
}

#define ENV_MAX_LENGTH	128
static char envbuf[ENV_MAX_LENGTH];

static int
new_job(struct user_arg_ptr envp, ya_thread_static_info_t *tsi)
{
        ya_job_attr_t  job_attr;
	int            job_attr_set;
        int            i;
        unsigned long *p;

	for(i = 0, job_attr_set = 0; job_attr_set != JOB_ATTR_ALL_SET; i++) {
                const char __user *ustr = get_user_arg_ptr(envp, i);
                if (!ustr)	// the end
			break;

                // EFAULT ignored xxx luke xxx
                if (strncpy_from_user(envbuf, ustr, ENV_MAX_LENGTH - 1) < 0)
                        break;

                p = (unsigned long *)envbuf;
		if (*p != *ya_zone_str)
                        continue;

		p = (unsigned long *)((char *)p + 9);
		if (*p == *zone_id_str) {
			p++;
			job_attr.zone = __search_zone_by_id(my_atol((const char *)p));
                        // EINVAL ignored xxx luke xxx
			if (!job_attr.zone)
                                break;

			job_attr_set |= ZONE_ID_SET;
		}
        }

        if (job_attr_set == JOB_ATTR_ALL_SET) {
                tsi->job_attr = job_attr;
                tsi->job      = ya_new_job(&tsi->job_attr);
                // ENOMEM ignored xxx luke xxx
                if (tsi->job) {
                        return NEW_JOB;
                }
        }

        return NON_JOB;
}

/*
 * If  ``current'' is  a process  of an  existing job,  save its  thread
 * information that will be restored into new created LDT after kernel's
 * execve().  Otherwise walk through  the environment variable array. If
 * ``current'' is the  root of a new job, a  new thread information data
 * area created, and after kernel's execve(), store the data into LDT.
 *
 *  xxx luke xxx
 *  TODO: from which version, ldt started not to be copied in execve()?
 */
static int
try_new_job(struct user_arg_ptr envp,  ya_thread_static_info_t *tsi)
{
        if (is_ya_thread(current)) {
                *tsi = *(get_tsi_task(current));
                return EXISTING_JOB;
        }

        return new_job(envp, tsi);
}

asmlinkage long
execve_agent(int fd, struct filename *filename,
             struct user_arg_ptr argv,
             struct user_arg_ptr envp,
             int flags)
{
        long ret;
        int  status;

        ya_thread_static_info_t	tsi;

	atomic64_add(EXECVE_YA_COUNT, &ya_count);

        status = try_new_job(envp, &tsi);

	ret = ((execve_func_t)orig_do_execve)(fd, filename, argv, envp, flags);

        if (status == NEW_JOB || status == EXISTING_JOB)
                init_job(&tsi);

	atomic64_sub(EXECVE_YA_COUNT, &ya_count);

	return ret;
}
