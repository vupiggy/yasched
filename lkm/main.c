/* -*- mode: C; c-file-style: "Linux" -*-
 *
 * Copyright (c) 2018 Luke Huang <lukehuang.ca@gmail.com>
 */

/**
 * \file main.c

 * \author Luke Huang
 */

#include <linux/module.h>
#include <linux/version.h>

#include "yacdev.h"
#include "inception.h"

#include "sched/sched.h"
#include "user/procfs.h"

/* xxx luke xxx
 * TODO: this is for:
 *    1. kallsyms_lookup_name
 *    2. probably sched_class or cgroup interfaces
 */
#if (LINUX_VERSION_CODE < KERNEL_VERSION(3,11,0))
MODULE_LICENSE("PROPRIETARY");
#else
MODULE_LICENSE("GPL");
#endif

int __init
yasched_init(void)
{
	int err;

	if ((err = create_char_device()))
		goto cdev_err;

	if ((err = init_procfs()))
		goto init_procfs_err;

	if ((err = init_sched()))
		goto init_sched_err;

	if ((err = perform_inception()))
		goto inception_err;

	printk("[YASched]: YASched is started\n");

	return 0;

inception_err:
	exit_sched();
init_sched_err:
        fini_procfs();
init_procfs_err:
	destroy_char_device();

cdev_err:
	return err;
}

void
yasched_exit(void)
{
	remove_inception();
	exit_sched();
	fini_procfs();
	destroy_char_device();

	printk("[YASched]: YASched is stopped\n");
}

module_init(yasched_init);
module_exit(yasched_exit);
