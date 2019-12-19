/* -*- mode: C; c-file-style: "Linux" -*-
 *
 * Copyright (c) 2018 Luke Huang <lukehuang.ca@gmail.com>
 *
 * $Id$
 */

/**
 * \file procfs_thread.c
 * \author Luke Huang
 */

#include <linux/sched.h>

static int
thread_seq_show(struct seq_file	*seq_file, void *v)
{
	ya_thread_t *thread = (void *)seq_file->private;

	seq_printf(seq_file, "Hello my name is: %s-%d\n",
                   thread->task->comm, thread->task->pid);

	return 0;
}

static int
thread_procfs_open(struct inode		*inode,
		   struct file		*file)
{
	/*
         * `single_open' is enough for now,  If we need output something
	 * larger than one page, use iterator instead.
	 */
	return single_open(file, thread_seq_show, PDE_DATA(inode));
}

static struct file_operations thread_procfs_ops = {
	.owner		= THIS_MODULE,
	.open		= thread_procfs_open,
	.read		= seq_read,
	.llseek		= seq_lseek,
	.release	= single_release
};
